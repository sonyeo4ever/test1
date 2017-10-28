


int trace_merge(char *config_name)
{
	FILE *config_fp;
	char *config_data;

    config_fp = fopen(config_name, "r");
    if (config_fp == NULL) {
		printf("error: cannot read %s\n", config_name);
		ret = -1;
		return -1;
	}

	fseek(config_fp, 0, SEEK_END);
	len = ftell(config_fp);
	fseek(config_fp, 0, SEEK_SET);

	config_data = (char *)malloc(len);
	if (config_data == NULL) {
		printf("error: failed malloc\n");
		fclose(config_fp);
		return -1;
	}
	fread(config_data, sizeof(char), len, config_fp);

	root_obj = cJSON_Parse(config_data);
	if (root_obj == NULL) {
		printf("error: %s\n", cJSON_GetErrorPtr());
		ret = -1;
		goto out;
	}




out:
	free(config_data);
	fclose(config_fp);
}


static 
