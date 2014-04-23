#include <stdio.h>
#include <stdlib.h>

struct vector {
        double  x,
                y,
                z;
};

struct arcInfo {
        struct vector	toSurf,
        		center,
        		cross;
        double	radius,
		angle,
		time;
};

struct lineInfo {
        struct vector	start,
        		end,
        		dir;
        double time;
};

struct rampInfo {
        struct vector   accel,
                        dir;
        double time;
};

int main (int argc, char **argv)
{
	FILE *file;
	int lineNum;
	struct arcInfo *arc;
	struct lineInfo *line;
	struct rampInfo ramp[2];

	if (argc < 2) {
		fprintf (stderr, "You need to give a file name\n");
		return 1;
	} else if (argc > 2) {
		fprintf (stderr, "Too many arguments\n");
		return 1;
	}

	file = fopen (argv[1], "r");

	fread (&lineNum, sizeof(int), 1, file);

	arc = malloc ((lineNum - 2) * sizeof(struct arcInfo));
	line = malloc ((lineNum - 1) * sizeof(struct lineInfo));

	fread (ramp, sizeof(struct rampInfo), 2, file);

	fread (arc, sizeof(struct arcInfo), lineNum - 2, file);
	fread (line, sizeof(struct lineInfo), lineNum - 1, file);

	fclose (file);

	free (arc);
	free (line);

	return 0;
}
