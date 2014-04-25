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
        struct vector accel;
        double time;
};

struct vector addVec (struct vector in1, struct vector in2)
{
        return (struct vector){in1.x + in2.x, in1.y + in2.y, in1.z + in2.z};
}

struct vector multVec (struct vector vec, double scaler)
{
        return (struct vector){vec.x * scaler, vec.y * scaler, vec.z * scaler};
}

double magnitude (struct vector input)
{
        return fabs (sqrt (pow (input.x, 2) + pow (input.y, 2) + pow (input.z, 2)));
}

struct vector getArcPos (struct arcInfo arc, double angle)
{
        double  cosAngle,
                sinAngle;

        cosAngle = cos (angle);
        sinAngle = sin (angle);

        return (struct vector){
                (cosAngle * arc.toSurf.x) + (sinAngle * arc.cross.x) + arc.center.x,
                (cosAngle * arc.toSurf.y) + (sinAngle * arc.cross.y) + arc.center.y,
                (cosAngle * arc.toSurf.z) + (sinAngle * arc.cross.z) + arc.center.z
        };
}

int main (int argc, char **argv)
{
	struct rampInfo ramp[2];
	struct lineInfo *line;
	struct arcInfo *arc;
	int lineNum;

	FILE *file;

	double time = 0.0;
        double totalTime = 0.0;
	double timeStep = 0.001;

        struct vector point = {0.0, 0.0, 0.0};
        struct vector lastPoint;

	FILE *posFile;
	FILE *timeFile;

	unsigned int i;

	if (argc < 2) {
		fprintf (stderr, "You need to give a file name\n");
		return 1;
	} else if (argc > 2) {
		fprintf (stderr, "Too many arguments\n");
		return 1;
	}

	if (!(file = fopen (argv[1], "r"))) {
		perror ("Failed to open file");
		exit (1);
	}

	fread (&lineNum, sizeof(int), 1, file);

	fread (ramp, sizeof(struct rampInfo), 2, file);

	if (!(arc = malloc ((lineNum - 2) * sizeof(struct arcInfo)))) {
		fprintf (stderr, "Failed to allocate memory\n");
		exit (1);
	}
	if (!(line = malloc ((lineNum - 1) * sizeof(struct lineInfo)))) {
		fprintf (stderr, "Failed to allocate memory\n");
		exit (1);
	}

	fread (arc, sizeof(struct arcInfo), lineNum - 2, file);
	fread (line, sizeof(struct lineInfo), lineNum - 1, file);

	fclose (file);

	posFile = fopen ("pos.csv", "w");
	timeFile = fopen ("time.csv", "w");

	//Ramp up
        for (time = 0.0; time < ramp[0].time; time += timeStep) {
		lastPoint = point;
		point = multVec (ramp[0].accel, 0.5 * pow (time, 2.0));
                fprintf (posFile, "%f, %f, %f\n", point.x, point.y, point.z);
                fprintf (timeFile, "%f, %f\n", totalTime + time, (point.x - lastPoint.x) / timeStep);
        }
        totalTime += ramp[0].time;

        for (i = 0; ; i++) {
		//Line
		for (time = 0.0; time < line[i].time; time += timeStep) {
			lastPoint = point;
			point = addVec (line[i].start, multVec (line[i].velocity, time));
			fprintf (posFile, "%f, %f, %f\n", point.x, point.y, point.z);
			fprintf (timeFile, "%f, %f\n", totalTime + time, (point.x - lastPoint.x) / timeStep);
		}
                totalTime += line[i].time;

                if (i == lineNum - 2)
                        break;

		//Arc
                for (time = 0.0; time < arc[i].time; time += timeStep) {
			lastPoint = point;
                        point = getArcPos (arc[i], ((0.5 * arc[i].accel * pow (time, 2.0)) + (magnitude (line[i].velocity) * time)) / arc[i].radius);
                        fprintf (posFile, "%f, %f, %f\n", point.x, point.y, point.z);
			fprintf (timeFile, "%f, %f\n", totalTime + time, (point.x - lastPoint.x) / timeStep);
                }
                totalTime += arc[i].time;
        }

	//Ramp down
        for (time = 0.0; time < ramp[1].time; time += timeStep) {
		lastPoint = point;
		point = addVec (line[lineNum - 2].end, addVec (multVec (ramp[1].accel, -0.5 * pow (time, 2.0)), multVec (line[lineNum - 2].velocity, time)));
                fprintf (posFile, "%f, %f, %f\n", point.x, point.y, point.z);
		fprintf (timeFile, "%f, %f\n", totalTime + time, (point.x - lastPoint.x) / timeStep);
        }
        totalTime += ramp[1].time;

	fclose (posFile);
	fclose (timeFile);

	free (arc);
	free (line);

	return 0;
}
