#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

	double time;
        double totalTime = 0.0;
        struct vector point;

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

        //Ramp up to first arc
        //Need better structure for this
        //Make more effieient
        for (time = 0; time < ramp[0].time; time += 0.001) {
                struct vector pos;
                //This has two vectors with the direction, only one is nessecary
                pos = multVec (&ramp[0].dir, ((magnitude (&ramp[0].accel) / 2.0) * pow (time, 2.0)) + (gCode[1].f * time));
                point = addVec (&gCode[0].p, &pos);
                printf ("%f, %f, %f\n", point.x, point.y, point.z);
        }
        totalTime += ramp[0].time;

        //Make sure there is no repeated point between lines and arcs
        //Make more efficient
        //Cleanup
        for (i = 0; ; i++) {
                for (time = 0; time < line[i].time; time += 0.001) {
                        struct vector pos;
                        pos = multVec (&line[i].dir, gCode[i + 1].f * time);
                        point = addVec (&line[i].start, &pos);
                        printf ("%f, %f, %f\n", point.x, point.y, point.z);
                }
                totalTime += line[i].time;

                if (i == lineNum - 2)
                        break;

                for (time = 0; time < arc[i].time; time += 0.001) {
                        point = getArcPos (&arc[i], ((((gCode[i + 2].f - gCode[i + 1].f) * pow (time, 2.0)) / (2.0 * arc[i].time)) + (gCode[i + 1].f * time)) / arc[i].radius);
                        printf ("%f, %f, %f\n", point.x, point.y, point.z);
                }
                totalTime += arc[i].time;
        }

        //Ramp down from last arc
        //Need better structure for this
        //Make more effieient
        for (time = 0; time < ramp[1].time; time += 0.001) {
                struct vector pos;
                //This has two vectors with the direction, only one is nessecary
                pos = multVec (&ramp[1].dir, ((magnitude (&ramp[1].accel) / 2.0) * pow (time, 2.0)) + (gCode[lineNum - 1].f * time));
                point = addVec (&line[2].end, &pos);
                printf ("%f, %f, %f\n", point.x, point.y, point.z);
        }

	free (arc);
	free (line);

	return 0;
}
