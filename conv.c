#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

//Check for bad gCode files!
//Function error checking
//Allow G02 and other commands
//Imposible path checking
//Make sure all varaible types are best for the situation

struct vector {
        double  x,
                y,
                z;
};

struct gCodeInfo {
        struct vector p;
        double f;
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

//Maximum accelerations allowed on the x, y, and z axies
const struct vector maxAccel = {
        400.0,
        300.0,
        200.0
};

struct vector addVec (struct vector *in1, struct vector *in2)
{
        return (struct vector){in1->x + in2->x, in1->y + in2->y, in1->z + in2->z};
}

struct vector subVec (struct vector *in1, struct vector *in2)
{
        return (struct vector){in1->x - in2->x, in1->y - in2->y, in1->z - in2->z};
}

struct vector multVec (struct vector *vec, double scaler)
{
        return (struct vector){vec->x * scaler, vec->y * scaler, vec->z * scaler};
}

double magnitude (struct vector *input)
{
        return fabs (sqrt (pow (input->x, 2) + pow (input->y, 2) + pow (input->z, 2)));
}

int normalize (struct vector *input)
{
        double mag;

        mag = magnitude (input);
        input->x /= mag;
        input->y /= mag;
        input->z /= mag;

        return 0;
}

struct vector crossProduct (struct vector *in1, struct vector *in2)
{
        return (struct vector){
                (in1->y * in2->z) - (in1->z * in2->y),
                (in1->z * in2->x) - (in1->x * in2->z),
                (in1->x * in2->y) - (in1->y * in2->x)
        };
}

//Use dot product instead
double angleProduct (struct vector *in1, struct vector *in2)
{
        struct vector cross;

        cross = crossProduct (&*in1, &*in2);

        return asin (magnitude (&cross) / (magnitude (&*in1) * magnitude (&*in2)));
}

//Make more efficient
struct vector getArcPos (struct arcInfo *arc, double angle)
{
        return (struct vector){
                (cos (angle) * arc->toSurf.x) + (sin (angle) * arc->cross.x) + arc->center.x,
                (cos (angle) * arc->toSurf.y) + (sin (angle) * arc->cross.y) + arc->center.y,
                (cos (angle) * arc->toSurf.z) + (sin (angle) * arc->cross.z) + arc->center.z
        };
}

int calcRampInfo (struct rampInfo *ramp, struct vector *start, struct vector *end, double startVel, double endVel, struct vector *line)
{
        ramp->accel = (struct vector){DBL_MAX, DBL_MAX, DBL_MAX};

        ramp->dir = subVec (&*end, &*start);
        normalize (&ramp->dir);

        if (start->x != end->x && fabs (ramp->accel.x) > maxAccel.x)
                ramp->accel = multVec (&ramp->dir, maxAccel.x / ramp->dir.x);
        if (start->y != end->y && fabs (ramp->accel.y) > maxAccel.y)
                ramp->accel = multVec (&ramp->dir, maxAccel.y / ramp->dir.y);
        if (start->z != end->z && fabs (ramp->accel.z) > maxAccel.z)
                ramp->accel = multVec (&ramp->dir, maxAccel.z / ramp->dir.z);

        ramp->time = fabs (endVel - startVel) / magnitude (&ramp->accel);

        //Make shorter
        if (endVel > startVel) {
                line->x = ((ramp->accel.x / 2.0) * pow (ramp->time, 2.0)) + (startVel * ramp->time) + start->x;
                line->y = ((ramp->accel.y / 2.0) * pow (ramp->time, 2.0)) + (startVel * ramp->time) + start->y;
                line->z = ((ramp->accel.z / 2.0) * pow (ramp->time, 2.0)) + (startVel * ramp->time) + start->z;
        } else {
                line->x = end->x - (((ramp->accel.x / 2.0) * pow (ramp->time, 2.0)) + (ramp->dir.x * startVel * ramp->time));
                line->y = end->y - (((ramp->accel.y / 2.0) * pow (ramp->time, 2.0)) + (ramp->dir.y * startVel * ramp->time));
                line->z = end->z - (((ramp->accel.z / 2.0) * pow (ramp->time, 2.0)) + (ramp->dir.z * startVel * ramp->time));
        }

        return 0;
}

/*
 * This subroutine calculates a travelable path based on the given G-code commands.
 * The path's corners are rounded based on the mills maximum accelerations.
 */
void calcArcInfo (struct gCodeInfo *gCode, struct arcInfo *arc, struct lineInfo *line, int lineNum)
{
	unsigned int i;

        for (i = 0; i < lineNum - 2; i++) {
		struct vector toCenter;
		struct vector normal;
		struct vector start;
		struct vector end;
		double centerMag;
		double length;
		double theta;

		double accel = DBL_MAX;

		//start and end are unit vectors pointing to the last and next G-code points from the current one
		start = subVec (&gCode[i].p, &gCode[i + 1].p);
		normalize (&start);
		end = subVec (&gCode[i + 2].p, &gCode[i + 1].p);
		normalize (&end);

		//toCenter is a unit vector pointing to where the center of the arc will be from the current G-code position
		toCenter = addVec (&start, &end);
		normalize (&toCenter);

		//theta is half the angle between start and end
		//angle is the angle that the arc will travel
		theta = angleProduct (&start, &toCenter);
		arc[i].angle = 2.0 * ((M_PI / 2.0) - theta);

		//Getting the minimum allowed acceleration of the axies in motion
		if (gCode[i + 1].p.x != gCode[i + 2].p.x && maxAccel.x < accel)
			accel = maxAccel.x;
		if (gCode[i + 1].p.y != gCode[i + 2].p.y && maxAccel.y < accel)
			accel = maxAccel.y;
		if (gCode[i + 1].p.z != gCode[i + 2].p.z && maxAccel.z < accel)
			accel = maxAccel.y;

		//Finding a radius that will not exceed the max acceleration, assuming the max accel is reached at the worst possible moment for the axis with the lowest allowed acceleration
		//Simplify equation
		arc[i].radius = sqrt (pow (((0.5 * pow (gCode[i + 2].f - gCode[i + 1].f, 2.0)) + ((gCode[i + 2].f - gCode[i + 1].f) * gCode[i + 1].f)) / (arc[i].angle * accel), 2.0) + pow (pow ((gCode[i + 2].f > gCode[i + 1].f)? gCode[i + 2].f : gCode[i + 1].f, 2.0) / accel, 2.0));

		//Finding the time spent traveling on the arc
		arc[i].time = (arc[i].angle * arc[i].radius) / (((gCode[i + 2].f - gCode[i + 1].f) / 2.0) + gCode[i + 1].f);

		//Calculating the actual acceleration
		arc[i].accel = (gCode[i + 2].f - gCode[i + 1].f) / arc[i].time;

		//Converting the unit vector to the center to a vector at the center
		centerMag = arc[i].radius / sin (theta);
		arc[i].center = multVec (&toCenter, centerMag);

		//Calculating the arc's start and end points
		length = arc[i].radius / tan (theta);
		line[i + 1].start = multVec (&end, length);
		line[i].end = multVec (&start, length);

		//Calculating a vector from the center to the start point
		arc[i].toSurf = subVec (&line[i].end, &arc[i].center);
		normalize (&arc[i].toSurf);

		//Moving the vectors from 000 to the current G-code position
		arc[i].center = addVec (&arc[i].center, &gCode[i + 1].p);
		line[i + 1].start = addVec (&line[i + 1].start, &gCode[i + 1].p);
		line[i].end = addVec (&line[i].end, &gCode[i + 1].p);

		//Calculating the cross unit vector, needed for the 3D equation of a circle
		normal = crossProduct (&end, &start);
		arc[i].cross = crossProduct (&normal, &arc[i].toSurf);
		normalize (&arc[i].cross);

		//Multiplying the circle equation unit vectors by the radius, needed for the 3D equation of a circle
		arc[i].toSurf = multVec (&arc[i].toSurf, arc[i].radius);
		arc[i].cross = multVec (&arc[i].cross, arc[i].radius);
	}
}

void calcLineInfo (struct gCodeInfo *gCode, struct lineInfo *line, int lineNum)
{
	unsigned int i;

        for (i = 0; i < lineNum - 1; i++) {
                line[i].velocity = subVec (&line[i].end, &line[i].start);
                line[i].time = magnitude (&line[i].velocity) / gCode[i + 1].f;
		normalize (&line[i].velocity);
		line[i].velocity = multVec (&line[i].velocity, gCode[i + 1].f);
        }
}

void readFile (char *fileName, struct gCodeInfo **gCode, int *lineNum)
{
	unsigned int i;
        char row[255];
        FILE *file;

        if (!(file = fopen (fileName, "r"))) {
		perror ("Failed to open file");
		exit (1);
	}

        for (*lineNum = 1; fgets (row, 255, file); (*lineNum)++);

	if (*lineNum < 3) {
		fprintf (stderr, "The G-code file needs to have more than one command\n");
		exit (1);
	}

        if (!(*gCode = malloc (*lineNum * sizeof(struct gCodeInfo)))) {
		fprintf (stderr, "Failed to allocate memory\n");
		exit (1);
	}
        *gCode[0] = (struct gCodeInfo){(struct vector){0.0, 0.0, 0.0}, 0.0};

        fseek (file, 0, SEEK_SET);

        for (i = 1; fgets (row, 255, file); i++) {
		static struct vector rowCode;
		double rowFeed = 0.0;
                char *split;

		split = strtok (row, " ");

                if (strcmp (split, "G01")) {
                        printf ("Unsupported G-code command on row %d\n", *lineNum);
                        break;
                }

                while (split) {
                        switch (split[0]) {
                                case 'X':
                                        rowCode.x = atof (++split);
                                        break;
                                case 'Y':
                                        rowCode.y = atof (++split);
                                        break;
                                case 'Z':
                                        rowCode.z = atof (++split);
                                        break;
                                case 'F':
					rowFeed = atof (++split);
                                        break;
                        }
                        split = strtok (NULL, " ");
                }

		if (rowFeed == 0.0) {
			fprintf (stderr, "Feed rate on line %d is missing, or zero\n", i);
			exit (1);
		}

		i[*gCode].p = rowCode;
		i[*gCode].f = rowFeed;

                free (split);
        }

        fclose (file);
}

void writeFile (struct rampInfo *ramp, struct arcInfo *arc, struct lineInfo *line, int lineNum, char *fileName)
{
	FILE *file;
	char *name;

	name = strtok (fileName, ".");

	file = fopen (strcat (name, ".bin"), "w");

	fwrite (&lineNum, sizeof(int), 1, file);
	fwrite (ramp, sizeof(struct rampInfo), 2, file);
	fwrite (arc, sizeof(struct arcInfo), lineNum - 2, file);
	fwrite (line, sizeof(struct lineInfo), lineNum - 1, file);

	fclose (file);
}

int main (int argc, char **argv)
{
        struct gCodeInfo *gCode;
        struct rampInfo ramp[2];
        struct lineInfo *line;
        struct arcInfo *arc;
        int lineNum;

	if (argc < 2) {
		fprintf (stderr, "You need to pass in a G-code file\n");
		return 1;
	} else if (argc > 2) {
		fprintf (stderr, "Too many arguments\n");
		return 1;
	}

	readFile (argv[1], &gCode, &lineNum);

        arc = malloc ((lineNum - 2) * sizeof(*arc));
        line = malloc ((lineNum - 1) * sizeof(*line));

        calcRampInfo (&ramp[0], &gCode[0].p, &gCode[1].p, gCode[0].f, gCode[1].f, &line[0].start);
        calcRampInfo (&ramp[1], &gCode[lineNum - 2].p, &gCode[lineNum - 1].p, gCode[lineNum - 1].f, 0.0, &line[lineNum - 2].end);
	calcArcInfo (gCode, arc, line, lineNum);
	calcLineInfo (gCode, line, lineNum);

        free (gCode);

	writeFile (ramp, arc, line, lineNum, argv[1]);

        free (arc);
        free (line);

        return 0;
}
