
#include "http.h"
#include "logger.h"

static http_response_t *home(const http_request_t *request);

static http_response_t *home(__unused const http_request_t *request)
{
        return create_response(200, "Kaldorei");
}

int main(void)
{
        server_config_t config = {
                .threads = 16,
                .port = 8080,
                .max_pending_requests = 100,
        };
        server_t *server = server_new(config);
        if (!server)
                log_fatal(-1, "Failed to create server");

        if (server_add_route(server, HTTP_GET, "/", home) < 0)
                log_fatal(-1, "Failed to add route");

        server_start(server);
        return 0;
}
