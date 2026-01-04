#include "mongoose.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Read hostname from system
static char *read_hostname(void)
{
	FILE *fp = popen("hostnamectl hostname", "r");
	if (fp == NULL)
		return NULL;

	char *hostname = malloc(256);
	if (hostname == NULL) {
		pclose(fp);
		return NULL;
	}

	if (fgets(hostname, 256, fp) == NULL) {
		free(hostname);
		pclose(fp);
		return NULL;
	}

	// Remove trailing newline
	size_t len = strlen(hostname);
	if (len > 0 && hostname[len - 1] == '\n') {
		hostname[len - 1] = '\0';
	}

	pclose(fp);
	return hostname;
}

// Set hostname using hostnamectl
static int set_hostname(const char *hostname)
{
	// Validate hostname: basic security check
	if (hostname == NULL || strlen(hostname) == 0 || strlen(hostname) > 253) {
		return -1;
	}

	// Check for dangerous characters that could be used for command injection
	for (const char *p = hostname; *p; p++) {
		if (*p == ';' || *p == '&' || *p == '|' || *p == '$' || *p == '`' || *p == '(' ||
		    *p == ')' || *p == '<' || *p == '>' || *p == '\n' || *p == '\r') {
			return -1;
		}
	}

	char cmd[512];
	snprintf(cmd, sizeof(cmd), "hostnamectl set-hostname %s", hostname);
	int result = system(cmd);
	return (result == 0) ? 0 : -1;
}

// Connection event handler function
static void ev_handler(struct mg_connection *c, int ev, void *ev_data)
{
	if (ev == MG_EV_HTTP_MSG) { // New HTTP request received
		struct mg_http_message *hm =
		    (struct mg_http_message *)ev_data; // Parsed HTTP request

		// GET /api/hostname - Read hostname
		if (mg_match(hm->method, mg_str("GET"), NULL) &&
		    mg_match(hm->uri, mg_str("/api/hostname"), NULL)) {
			char *hostname = read_hostname();
			if (hostname != NULL) {
				mg_http_reply(c, 200, "Content-Type: application/json\r\n",
				              "{%m:%m}\n", MG_ESC("hostname"), MG_ESC(hostname));
				free(hostname);
			} else {
				mg_http_reply(c, 500, "Content-Type: application/json\r\n",
				              "{%m:%m}\n", MG_ESC("error"),
				              MG_ESC("Failed to read hostname"));
			}
		}
		// POST /api/hostname - Set hostname
		else if (mg_match(hm->method, mg_str("POST"), NULL) &&
		         mg_match(hm->uri, mg_str("/api/hostname"), NULL)) {
			// Parse JSON body
			char *hostname = mg_json_get_str(hm->body, "$.hostname");
			if (hostname != NULL) {
				int result = set_hostname(hostname);
				if (result == 0) {
					mg_http_reply(c, 200, "Content-Type: application/json\r\n",
					              "{%m:%m}\n", MG_ESC("status"),
					              MG_ESC("success"));
				} else {
					mg_http_reply(c, 400, "Content-Type: application/json\r\n",
					              "{%m:%m,%m:%m}\n", MG_ESC("status"),
					              MG_ESC("error"), MG_ESC("message"),
					              MG_ESC("Invalid hostname or failed to set"));
				}
				free(hostname);
			} else {
				mg_http_reply(c, 400, "Content-Type: application/json\r\n",
				              "{%m:%m,%m:%m}\n", MG_ESC("status"), MG_ESC("error"),
				              MG_ESC("message"),
				              MG_ESC("Missing or invalid hostname in request"));
			}
		}
		// Existing /api/hello endpoint
		else if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {
			mg_http_reply(c, 200, "", "{%m:%d}\n", MG_ESC("status"), 1);
		}
		// Serve static files (index.html should be in the same directory as executable)
		else {
			struct mg_http_serve_opts opts = {.root_dir = ".", .fs = &mg_fs_posix};
			mg_http_serve_dir(c, hm, &opts);
		}
	}
}

int main()
{
	struct mg_mgr mgr; // Mongoose event manager. Holds all connections
	mg_mgr_init(&mgr); // Initialise event manager
	mg_http_listen(&mgr, "http://0.0.0.0:8000", ev_handler, NULL);
	mg_http_listen(&mgr, "https://0.0.0.0:8443", ev_handler, NULL);
	for (;;) {
		mg_mgr_poll(&mgr, 1000); // Infinite event loop
	}
	return 0;
}
