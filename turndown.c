#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
#include <ao/ao.h>

#define DELAY 10000

static void turnup() {
	(void)system("osascript -e 'set volume 100'");
}

static int turndown(char *filename, char *boundary, void (*func)()) {
	int rc = 0, boundaries = 0, i = 0, l = 0;
	int header_size = 44, default_driver = 0, f_size = 0, b_count = 0;
	size_t file_size = 0, boundary_offset = 0, boundary_size = 0, wav_size = 0;
	size_t BUF_SIZE = 4096, leftover_bytes = 0;
	char *exec = NULL, *cmd_str = NULL, *cmd_which = NULL, *wav = NULL, *buffer = NULL;
	FILE *file = NULL, *cmd = NULL;
	ao_device *device;
	ao_sample_format format;
	struct wav_header {
		int format;
		int channels;
		int frequency;
		int bytesInData;
	};

	struct wav_header header;

	if (!filename || strlen(filename) == 0) {
		printf("No filename provided\n");

		rc = 1;
		goto cleanup;
	}

	if (strncmp(filename, ".", 1) != 0 && strncmp(filename, "/", 1) != 0) {
		cmd_str = malloc(sizeof(char) * (7 + strlen(filename)));
		strcpy(cmd_str, "which ");
		strcat(cmd_str, filename);

		cmd = popen(cmd_str, "r");
		cmd_which = malloc(sizeof(char) * 100);
		(void)fread(cmd_which, 1, 100, cmd);

		for (i = (int)strlen(cmd_which), l = 0; i >= l; i--) {
			if (cmd_which[i] == '\n') {
				cmd_which[i] = '\0';
				break;
			}
		}

		filename = cmd_which;
	}

	file = fopen(filename, "rb");

	if (!file) {
		printf("Could not open %s\n", filename);

		rc = 1;
		goto cleanup;
	}

	(void)fseek(file, 0, SEEK_END);
	file_size = (size_t)ftell(file);
	(void)fseek(file, 0, SEEK_SET);

	if (file_size == 0) {
		printf("%s is empty\n", filename);

		rc = 1;
		goto cleanup;
	}

	exec = malloc(sizeof(char) * file_size + 1);

	if (!exec) {
		printf("Memory error\n");

		rc = 1;
		goto cleanup;
	}

	if (fread(exec, 1, file_size, file) != file_size) {
		printf("Could not read %s\n", filename);

		rc = 1;
		goto cleanup;
	}

	boundary_size = strlen(boundary);

	for (i = 0, l = (int)file_size; i < l; i++) {
		if (strncmp(exec + i, boundary, boundary_size) == 0) {
			boundaries++;
		}

		if (boundaries == 2) {
			boundary_offset = i + boundary_size;
			break;
		}
	}

	wav_size = file_size - boundary_offset;
	wav = malloc(sizeof(char) * wav_size);
	memcpy(wav, exec + boundary_offset, wav_size);

	if (strncmp(wav, "RIFF", 4) != 0) {
		printf("Invalid WAV header\n");

		rc = 1;
		goto cleanup;
	}

	if (strncmp(wav + 8, "WAVEfmt ", 8) != 0) {
		printf("Invalid WAVE fmt\n");

		rc = 1;
		goto cleanup;
	}

	header.format = (int)wav[16];
	header.channels = (int)wav[22];
	header.frequency = (int)wav[24] + ((int)wav[25] << 8) + ((int)wav[26] << 16) + ((int)wav[27] << 24);

	if (strncmp(wav + 36, "data", 4) != 0) {
		printf("Invalid data\n");

		rc = 1;
		goto cleanup;
	}

	header.bytesInData = ((int)wav[40] & 0xff) + (((int)wav[41] & 0xff) << 8) + (((int)wav[42] & 0xff) << 16) + (((int)wav[43] & 0xff) << 24);

	ao_initialize();
	default_driver = ao_default_driver_id();

	memset(&format, 0, sizeof(format));
	format.bits = header.format;
	format.channels = header.channels;
	format.rate = header.frequency;
	format.byte_format = AO_FMT_LITTLE;

	device = ao_open_live(default_driver, &format, NULL);

	if (device == NULL) {
		printf("Could not open device\n");

		rc = 1;
		goto cleanup;
	}

	f_size = header.bytesInData;
	b_count = (int)(f_size / BUF_SIZE);

	for (i = 0, l = b_count; i < l; i++) {
		(void)ao_play(device, wav + header_size + (i * BUF_SIZE), (uint_32)BUF_SIZE);
		(*func)();
	}

	leftover_bytes = (size_t)(file_size % BUF_SIZE);

	if (leftover_bytes != 0) {
		buffer = malloc(sizeof(char) * BUF_SIZE);
		memcpy(buffer, wav + header_size + (i * BUF_SIZE), leftover_bytes);
		memset(buffer + leftover_bytes, 0, BUF_SIZE - leftover_bytes);
		(void)ao_play(device, buffer, (uint_32)BUF_SIZE);
	}

	(void)ao_close(device);
	ao_shutdown();

cleanup:
	if (file) {
		(void)fclose(file);
	}

	if (exec) {
		free(exec);
	}

	if (wav) {
		free(wav);
	}

	if (buffer) {
		free(buffer);
	}

	if (cmd) {
		(void)pclose(cmd);
	}

	if (cmd_str) {
		free(cmd_str);
	}

	if (cmd_which) {
		free(cmd_which);
	}

	return rc;
}

static void forwhat() {
	printf("TURN DOWN FOR WHAT?!\n");
}

static int check_volume() {
	int volume = 0, i = 0, l = 0;
	size_t settings_length = 74;
	char *settings = NULL;
	FILE *cmd = NULL;

	cmd = popen("osascript -e 'get volume settings'", "r");

	if (!cmd) {
		printf("Could not execute command\n");
		goto cleanup;
	}

	settings = malloc(sizeof(char) * settings_length);
	(void)fread(settings, 1, settings_length, cmd);

	sscanf(settings, "output volume:%d,", &volume);

	if (volume != 0) {
		for (i = (int)settings_length, l = 0; i >= l; i--) {
			if (settings[i] == 'e') {
				settings[i + 1] = '\0';
				break;
			}
		}

		settings_length = strlen(settings);

		if (strncmp(settings + (settings_length - 4), "true", 4) == 0) {
			volume = 0;
		}
	}

cleanup:
	if (settings) {
		free(settings);
	}

	if (cmd) {
		(void)pclose(cmd);
	}

	return volume;
}

int main(int argc, char *argv[]) {
	int repeat = 1;
	char *exec = argv[0];

	while (repeat == 1) {
		if (check_volume() == 0) {
			(void)system("clear");
			turnup();

			if (turndown(exec, "turndown.wav", &forwhat) != 0) {
				printf("Could not turn down\n");
			}
		}

		//usleep(DELAY);
	}

	return 0;
}