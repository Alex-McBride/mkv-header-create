all: mkv-header-create.c
	gcc mkv-header-create.c header-settings.c context.c cues.c -g -Wall -o mkv-header-create
