#include "app_config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#define PLACEHOLDER "@LOG_FILE@"

static void set_err(char *errbuf, size_t errbuf_sz, const char *msg);

static int is_valid_pinctrl_func(const char *func)
{
	if (func == NULL) {
		return 0;
	}
	if (strcmp(func, "ip") == 0 || strcmp(func, "op") == 0 ||
	    strcmp(func, "no") == 0) {
		return 1;
	}
	if (func[0] == 'a' && func[1] >= '0' && func[1] <= '8' && func[2] == '\0') {
		return 1;
	}
	return 0;
}

static int parse_pinctrl_config(const cJSON *root, piduier_config_t *cfg,
                                char *errbuf, size_t errbuf_sz)
{
	cJSON *pinctrl = cJSON_GetObjectItemCaseSensitive((cJSON *)root, "pinctrl");
	if (!cJSON_IsObject(pinctrl)) {
		set_err(errbuf, errbuf_sz, "pinctrl object is required");
		return -1;
	}

	for (int i = 0; i < PIDUIER_PINCTRL_GPIO_COUNT; i++) {
		char key[16];
		snprintf(key, sizeof(key), "gpio%d", i);
		cJSON *j_func = cJSON_GetObjectItemCaseSensitive(pinctrl, key);
		if (!cJSON_IsString(j_func) || j_func->valuestring == NULL) {
			snprintf(errbuf, errbuf_sz,
			         "pinctrl.%s is required and must be a string", key);
			return -1;
		}
		if (!is_valid_pinctrl_func(j_func->valuestring)) {
			snprintf(errbuf, errbuf_sz,
			         "pinctrl.%s must be one of ip/op/a0~a8/no", key);
			return -1;
		}
		if (strlen(j_func->valuestring) >= PIDUIER_PINCTRL_FUNC_MAX_LEN) {
			snprintf(errbuf, errbuf_sz, "pinctrl.%s is too long", key);
			return -1;
		}
		strcpy(cfg->pinctrl[i], j_func->valuestring);
	}

	const cJSON *ch;
	cJSON_ArrayForEach(ch, pinctrl)
	{
		if (ch->string == NULL) {
			set_err(errbuf, errbuf_sz, "pinctrl contains an unnamed key");
			return -1;
		}
		if (strncmp(ch->string, "gpio", 4) != 0) {
			snprintf(errbuf, errbuf_sz, "pinctrl contains invalid key: %s",
			         ch->string);
			return -1;
		}
		char *end = NULL;
		long idx = strtol(ch->string + 4, &end, 10);
		if (end == NULL || *end != '\0' || idx < 0 ||
		    idx >= PIDUIER_PINCTRL_GPIO_COUNT) {
			snprintf(errbuf, errbuf_sz, "pinctrl contains invalid key: %s",
			         ch->string);
			return -1;
		}
		if (!cJSON_IsString(ch) || ch->valuestring == NULL ||
		    !is_valid_pinctrl_func(ch->valuestring)) {
			snprintf(errbuf, errbuf_sz,
			         "pinctrl.%s must be one of ip/op/a0~a8/no", ch->string);
			return -1;
		}
	}

	return 0;
}

static void set_err(char *errbuf, size_t errbuf_sz, const char *msg)
{
	if (errbuf != NULL && errbuf_sz > 0) {
		snprintf(errbuf, errbuf_sz, "%s", msg);
	}
}

static char *read_file_all(const char *path, char *errbuf, size_t errbuf_sz)
{
	FILE *fp = fopen(path, "rb");
	if (fp == NULL) {
		snprintf(errbuf, errbuf_sz, "cannot open config file: %s", path);
		return NULL;
	}
	if (fseek(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		set_err(errbuf, errbuf_sz, "fseek failed on config file");
		return NULL;
	}
	long sz = ftell(fp);
	if (sz < 0) {
		fclose(fp);
		set_err(errbuf, errbuf_sz, "ftell failed on config file");
		return NULL;
	}
	rewind(fp);
	char *buf = (char *)malloc((size_t)sz + 1);
	if (buf == NULL) {
		fclose(fp);
		set_err(errbuf, errbuf_sz, "out of memory reading config");
		return NULL;
	}
	size_t n = fread(buf, 1, (size_t)sz, fp);
	fclose(fp);
	buf[n] = '\0';
	return buf;
}

static int rule_needs_log_path(const char *rule)
{
	return strstr(rule, PLACEHOLDER) != NULL;
}

static int any_rule_needs_log_path(const cJSON *rules)
{
	const cJSON *r;
	cJSON_ArrayForEach(r, rules)
	{
		if (cJSON_IsString(r) && r->valuestring != NULL &&
		    rule_needs_log_path(r->valuestring)) {
			return 1;
		}
	}
	return 0;
}

/** Escape path for use inside zlog double-quoted file segment (only \ and ").
 */
static char *escape_zlog_path(const char *path, char *errbuf, size_t errbuf_sz)
{
	size_t n = strlen(path);
	size_t extra = 0;
	for (size_t i = 0; i < n; i++) {
		if (path[i] == '\\' || path[i] == '"') {
			extra++;
		}
	}
	char *out = (char *)malloc(n + extra + 1);
	if (out == NULL) {
		set_err(errbuf, errbuf_sz, "out of memory");
		return NULL;
	}
	size_t j = 0;
	for (size_t i = 0; i < n; i++) {
		if (path[i] == '\\' || path[i] == '"') {
			out[j++] = '\\';
		}
		out[j++] = path[i];
	}
	out[j] = '\0';
	return out;
}

static char *substitute_log_file(const char *rule, const char *log_file,
                                 char *errbuf, size_t errbuf_sz)
{
	const char *p = strstr(rule, PLACEHOLDER);
	if (p == NULL) {
		return strdup(rule);
	}
	char *escaped = escape_zlog_path(log_file, errbuf, errbuf_sz);
	if (escaped == NULL) {
		return NULL;
	}
	size_t before = (size_t)(p - rule);
	size_t ph_len = strlen(PLACEHOLDER);
	size_t rest_len = strlen(p + ph_len);
	size_t new_len = before + strlen(escaped) + rest_len;
	char *out = (char *)malloc(new_len + 1);
	if (out == NULL) {
		free(escaped);
		set_err(errbuf, errbuf_sz, "out of memory");
		return NULL;
	}
	memcpy(out, rule, before);
	memcpy(out + before, escaped, strlen(escaped));
	memcpy(out + before + strlen(escaped), p + ph_len, rest_len + 1);
	free(escaped);
	return out;
}

static int check_object_all_strings(const cJSON *obj, const char *ctx,
                                    char *errbuf, size_t errbuf_sz)
{
	if (!cJSON_IsObject(obj)) {
		snprintf(errbuf, errbuf_sz, "%s must be a JSON object", ctx);
		return -1;
	}
	const cJSON *ch;
	cJSON_ArrayForEach(ch, obj)
	{
		if (!cJSON_IsString(ch) || ch->valuestring == NULL) {
			snprintf(errbuf, errbuf_sz,
			         "%s: value for key \"%s\" must be a string", ctx,
			         ch->string ? ch->string : "?");
			return -1;
		}
	}
	return 0;
}

static int validate_http_port(const cJSON *item, char *errbuf, size_t errbuf_sz)
{
	if (!cJSON_IsNumber(item)) {
		set_err(errbuf, errbuf_sz, "http_port must be a number");
		return -1;
	}
	double d = item->valuedouble;
	if (isnan(d) || d < 1.0 || d > 65535.0) {
		set_err(errbuf, errbuf_sz, "http_port must be between 1 and 65535");
		return -1;
	}
	int port = (int)d;
	if ((double)port != d) {
		set_err(errbuf, errbuf_sz, "http_port must be an integer");
		return -1;
	}
	return port;
}

void piduier_config_free(piduier_config_t *cfg)
{
	if (cfg == NULL) {
		return;
	}
	free(cfg->zlog_ini);
	cfg->zlog_ini = NULL;
}

int piduier_config_load(const char *path, piduier_config_t *cfg, char *errbuf,
                        size_t errbuf_sz)
{
	if (path == NULL || cfg == NULL) {
		set_err(errbuf, errbuf_sz, "invalid arguments");
		return -1;
	}
	memset(cfg, 0, sizeof(*cfg));
	strcpy(cfg->web_root, "./web");

	char *json_text = read_file_all(path, errbuf, errbuf_sz);
	if (json_text == NULL) {
		return -1;
	}

	cJSON *root = cJSON_Parse(json_text);
	free(json_text);
	if (root == NULL) {
		const char *ep = cJSON_GetErrorPtr();
		if (ep != NULL) {
			snprintf(errbuf, errbuf_sz, "JSON parse error near: %.40s", ep);
		} else {
			set_err(errbuf, errbuf_sz, "JSON parse error");
		}
		return -1;
	}

	int rc = -1;

	cJSON *j_listen = cJSON_GetObjectItemCaseSensitive(root, "http_listen");
	if (!cJSON_IsString(j_listen) || j_listen->valuestring == NULL ||
	    j_listen->valuestring[0] == '\0') {
		set_err(errbuf, errbuf_sz,
		        "http_listen is required and must be a non-empty string");
		goto done;
	}
	if (strlen(j_listen->valuestring) >= sizeof(cfg->http_listen)) {
		set_err(errbuf, errbuf_sz, "http_listen is too long");
		goto done;
	}
	strcpy(cfg->http_listen, j_listen->valuestring);

	cJSON *j_port = cJSON_GetObjectItemCaseSensitive(root, "http_port");
	int port = validate_http_port(j_port, errbuf, errbuf_sz);
	if (port < 0) {
		goto done;
	}
	cfg->http_port = port;

	cJSON *j_web_root = cJSON_GetObjectItemCaseSensitive(root, "web_root");
	if (j_web_root != NULL) {
		if (!cJSON_IsString(j_web_root) || j_web_root->valuestring == NULL ||
		    j_web_root->valuestring[0] == '\0') {
			set_err(errbuf, errbuf_sz, "web_root must be a non-empty string");
			goto done;
		}
		if (strlen(j_web_root->valuestring) >= sizeof(cfg->web_root)) {
			set_err(errbuf, errbuf_sz, "web_root is too long");
			goto done;
		}
		strcpy(cfg->web_root, j_web_root->valuestring);
	}

	if (parse_pinctrl_config(root, cfg, errbuf, errbuf_sz) != 0) {
		goto done;
	}

	cJSON *zlog = cJSON_GetObjectItemCaseSensitive(root, "zlog");
	if (!cJSON_IsObject(zlog)) {
		set_err(errbuf, errbuf_sz, "zlog object is required");
		goto done;
	}

	cJSON *j_log_file = cJSON_GetObjectItemCaseSensitive(zlog, "log_file");
	if (!cJSON_IsString(j_log_file) || j_log_file->valuestring == NULL) {
		set_err(errbuf, errbuf_sz,
		        "zlog.log_file is required and must be a string");
		goto done;
	}
	if (strlen(j_log_file->valuestring) >= sizeof(cfg->log_file)) {
		set_err(errbuf, errbuf_sz, "zlog.log_file is too long");
		goto done;
	}
	strcpy(cfg->log_file, j_log_file->valuestring);

	cJSON *global = cJSON_GetObjectItemCaseSensitive(zlog, "global");
	if (check_object_all_strings(global, "zlog.global", errbuf, errbuf_sz) !=
	    0) {
		goto done;
	}

	cJSON *formats = cJSON_GetObjectItemCaseSensitive(zlog, "formats");
	if (check_object_all_strings(formats, "zlog.formats", errbuf, errbuf_sz) !=
	    0) {
		goto done;
	}

	cJSON *rules = cJSON_GetObjectItemCaseSensitive(zlog, "rules");
	if (!cJSON_IsArray(rules)) {
		set_err(errbuf, errbuf_sz, "zlog.rules must be a JSON array");
		goto done;
	}
	if (cJSON_GetArraySize(rules) < 1) {
		set_err(errbuf, errbuf_sz, "zlog.rules must contain at least one rule");
		goto done;
	}
	const cJSON *r;
	cJSON_ArrayForEach(r, rules)
	{
		if (!cJSON_IsString(r) || r->valuestring == NULL) {
			set_err(errbuf, errbuf_sz,
			        "each zlog.rules[] entry must be a string");
			goto done;
		}
	}

	int need_path = any_rule_needs_log_path(rules);
	if (need_path && cfg->log_file[0] == '\0') {
		set_err(
		    errbuf, errbuf_sz,
		    "zlog.log_file must be non-empty when rules contain @LOG_FILE@");
		goto done;
	}

	char *mem = NULL;
	size_t mem_sz = 0;
	FILE *out = open_memstream(&mem, &mem_sz);
	if (out == NULL) {
		set_err(errbuf, errbuf_sz, "open_memstream failed");
		goto done;
	}

	fprintf(out, "[global]\n");
	const cJSON *g;
	cJSON_ArrayForEach(g, global)
	{
		if (g->string == NULL) {
			fclose(out);
			free(mem);
			set_err(errbuf, errbuf_sz, "zlog.global: missing key name");
			goto done;
		}
		fprintf(out, "%s = %s\n", g->string, g->valuestring);
	}

	fprintf(out, "\n[formats]\n");
	const cJSON *f;
	cJSON_ArrayForEach(f, formats)
	{
		if (f->string == NULL) {
			fclose(out);
			free(mem);
			set_err(errbuf, errbuf_sz, "zlog.formats: missing key name");
			goto done;
		}
		fprintf(out, "%s = %s\n", f->string, f->valuestring);
	}

	fprintf(out, "\n[rules]\n");
	cJSON_ArrayForEach(r, rules)
	{
		char *line = substitute_log_file(r->valuestring, cfg->log_file, errbuf,
		                                 errbuf_sz);
		if (line == NULL) {
			fclose(out);
			free(mem);
			goto done;
		}
		fprintf(out, "%s\n", line);
		free(line);
	}

	if (fclose(out) != 0) {
		free(mem);
		set_err(errbuf, errbuf_sz, "failed to finalize zlog config buffer");
		goto done;
	}

	cfg->zlog_ini = mem;
	rc = 0;

done:
	cJSON_Delete(root);
	if (rc != 0) {
		piduier_config_free(cfg);
		memset(cfg, 0, sizeof(*cfg));
	}
	return rc;
}
