#ifndef http_request_H
#define http_request_H

/**
 * @brief  Requests data from the weather api.
 */
void api_request(void);

/**
 * @brief  Gets the http response.
 */
char* http_request_get_response(void);

#endif