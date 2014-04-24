#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//Error checking

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
		accel,
		time;
};

struct lineInfo {
        struct vector	velocity,
        		start,
        		end;
        double time;
};

struct rampInfo {
        struct vector   accel,
                        dir;
        double time;
};

struct vector addVec (struct vector *in1, struct vector *in2)
{
        return (struct vector){in1->x + in2->x, in1->y + in2->y, in1->z + in2->z};
}

struct vector multVec (struct vector *vec, double scaler)
{
        return (struct vector){vec->x * scaler, vec->y * scaler, vec->z * scaler};
}

double magnitude (struct vector *input)
{
        return fabs (sqrt (pow (input->x, 2) + pow (input->y, 2) + pow (input->z, 2)));
}

struct vector getArcPos (struct arcInfo *arc, double angle)
{
        return (struct vector){
                (cos (angle) * arc->toSurf.x) + (sin (angle) * arc->cross.x) + arc->center.x,
                (cos (angle) * arc->toSurf.y) + (sin (angle) * arc->cross.y) + arc->center.y,
                (cos (angle) * arc->toSurf.z) + (sin (angle) * arc->cross.z) + arc->center.z
        };
}

int main (int argc, char **argv)
{
	FILE *file;
	int lineNum;
	struct arcInfo *arc;
	struct lineInfo *line;
	struct rampInfo ramp[2];

	double time = 0;
        double totalTime = 0;
	double timeStep = 0.001;
        struct vector point;

	unsigned int i;

	if (argc < 2) {
		fprintf (stderr, "You need to give a file name\n");
		return 1;
	} else if (argc > 2) {
		fprintf (stderr, "Too many arguments\n");
		return 1;
	}

	file = fopen (argv[1], "r");

	fread (&lineNum, sizeof(int), 1, file);

	fread (ramp, sizeof(struct rampInfo), 2, file);

	arc = malloc ((lineNum - 2) * sizeof(struct arcInfo));
	line = malloc ((lineNum - 1) * sizeof(struct lineInfo));

	fread (arc, sizeof(struct arcInfo), lineNum - 2, file);
	fread (line, sizeof(struct lineInfo), lineNum - 1, file);

	fclose (file);

	file = fopen ("plot.csv", "w");

        for (i = 0; ; i++) {
		for (time = 0.0; time < line[i].time; time += timeStep) {
			point = multVec (&line[i].velocity, time);
			point = addVec (&line[i].start, &point);
			fprintf (file, "%f, %f, %f\n", point.x, point.y, point.z);
		}
                totalTime += line[i].time;

                if (i == lineNum - 2)
                        break;

                for (time = 0.0; time < arc[i].time; time += timeStep) {
                        point = getArcPos (&arc[i], ((0.5 * arc[i].accel * pow (time, 2.0)) + (magnitude (&line[i].velocity) * time)) / arc[i].radius);
                        fprintf (file, "%f, %f, %f\n", point.x, point.y, point.z);

                }
                totalTime += arc[i].time;
        }

	fclose (file);

	free (arc);
	free (line);

	return 0;
}
