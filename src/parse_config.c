#define _GNU_SOURCE
#include "parse_config.h"

proxy_config_t* get_config(char *filename)
{
    FILE *file = fopen(filename, "rt");

    proxy_config_t *p_config = malloc(sizeof(proxy_config_t));
    char *string[256];
    char delimiter[] = " \n";
    if (file != NULL)
    {
        char line[MAXBUF];
        memset(&line, 0, sizeof(line));
        while (fgets(line, sizeof(line), file) != NULL)
        {
            string[0] = strtok(line, delimiter);
            if(!string[0])
            {
            	memset(&line, 0, sizeof(line));
            	continue;
            }
            int i = 0;
            while (string[i] != NULL)
            {
                i++;
                string[i] = strtok(NULL, delimiter);

            }
            if (strncmp(string[0], "TCP_RECEIVE_SERVER", 18) == 0)
            {
                uv_ip4_addr(string[1], atoi(string[2]), &p_config->tcp_server_sequence_addr);
            }
            else if (strncmp(string[0], "TCP_PROXY", 9) == 0)
            {
                uv_ip4_addr(string[1], atoi(string[2]), &p_config->tcp_proxy_server_addr);
            }
            else if (strncmp(string[0], "TCP", 3) == 0)
            {
                uv_ip4_addr(string[1], atoi(string[2]), &p_config->tcp_server_addr);
            }
            else if (strncmp(string[0], "UDP", 3) == 0)
            {
                uv_ip4_addr(string[1], atoi(string[2]), &p_config->udp_server_addr);
                if((i-3)%2 != 0)
                {
                	memset(&line, 0, sizeof(line));
                	continue;
                }
                p_config->numOfUDPClient = (i - 3)/2;
                p_config->udp_client_addr = malloc(p_config->numOfUDPClient * sizeof(struct sockaddr_in));
                for(i=0 ; i<p_config->numOfUDPClient ; i++)
                {
                	uv_ip4_addr(string[i*2+3], atoi(string[i*2+4]), &p_config->udp_client_addr[i]);
                }
            }
            else if (strncmp(string[0], "TIME_GET_FILE", 13) == 0)
            {
                p_config->timeGetFile = malloc((i-1) * sizeof(struct tm));
                init_timer(string+1, p_config->timeGetFile);
                p_config->ntimeGetFile = i-1;
            }
            else if (strncmp(string[0], "TIME_GET_SC", 11) == 0)
            {
                p_config->timeGetSC = malloc((i-1) * sizeof(time_t));
                init_timer(string+1, p_config->timeGetSC);
                p_config->ntimeGetSC = i-1;
            }
            else if (strncmp(string[0], "FILE_PATH", 9) == 0)
            {
                p_config->filePath = strdup(string[1]);
            }
			else if (strncmp(string[0], "ACCEPT_CLIENT", 13) == 0)
			{
			   init_filter(p_config->filter, string);
			}
            memset(&line, 0, sizeof(line));
        }

        fclose(file);
    } // End if file
    return p_config;
}

void init_timer(char *info[256], struct tm *timeConfiguration)
{
    unsigned int i = 0;
    while (info[i] != NULL)
    {
        strptime(info[i], "%H:%M", &timeConfiguration[i]);
        i++;
    }
}

void init_filter(char *filter[], char *string[256])
{
    unsigned int i = 1;
    while (string[i] != NULL)
    {

        filter[i-1] = strdup(string[i]);
        logger_info("ACCEPTED IP-----------%s\n",filter[i-1]);
        i++;
    }
}

