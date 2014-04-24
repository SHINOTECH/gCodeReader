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
//Change from double to float?

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

double angleProduct (struct vector *in1, struct vector *in2)
{
        struct vector cross;

        cross = crossProduct (&*in1, &*in2);

        return asin (magnitude (&cross) / (magnitude (&*in1) * magnitude (&*in2)));
}

struct vector getArcPos (struct arcInfo *arc, double angle)
{
        return (struct vector){
                (cos (angle) * arc->toSurf.x) + (sin (angle) * arc->cross.x) + arc->center.x,
                (cos (angle) * arc->toSurf.y) + (sin (angle) * arc->cross.y) + arc->center.y,
                (cos (angle) * arc->toSurf.z) + (sin (angle) * arc->cross.z) + arc->center.z
        };
}

int getRampInfo (struct rampInfo *ramp, struct vector *start, struct vector *end, double startVel, double endVel, struct vector *line)
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

/* Reads the G-code file into an array */
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

void writeFile (struct rampInfo *ramp, struct arcInfo *arc, struct lineInfo *line, int *lineNum, char *fileName)
{
	FILE *file;
	char *name;

	name = strtok (fileName, ".");

	file = fopen (strcat (name, ".bin"), "w");

	fwrite (lineNum, sizeof(int), 1, file);
	fwrite (ramp, sizeof(struct rampInfo), 2, file);
	fwrite (arc, sizeof(struct arcInfo), *lineNum - 2, file);
	fwrite (line, sizeof(struct lineInfo), *lineNum - 1, file);

	fclose (file);
}

//Put each component into a subroutine
int main (int argc, char **argv)
{
        //Clean up the variables
        unsigned int i;
        unsigned int lineNum;
        struct gCodeInfo *gCode;

        struct arcInfo *arc;
        struct lineInfo *line;

        struct rampInfo ramp[2];

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

        //Clean up
        //Shift i down one?
        //Improve and add comments
        for (i = 1; i < lineNum - 1; i++) {
                struct vector toCenter;
                struct vector normal;
                struct vector start;
                struct vector end;
                double centerMag;
                double length;
                double theta;

                double accel = DBL_MAX;

                //start and end are unit vectors pointing to the last and next G-code points from the current one
                start = subVec (&gCode[i - 1].p, &gCode[i].p);
                end = subVec (&gCode[i + 1].p, &gCode[i].p);
                normalize (&start);
                normalize (&end);

                //toCenter is a unit vector pointing to where the center of the arc will be from the current G-code position
                toCenter = addVec (&start, &end);
                normalize (&toCenter);

                //theta is half the angle between start and end
                //angle is the angle that the arc will travel
                theta = angleProduct (&start, &toCenter);
                arc[i - 1].angle = 2.0 * ((M_PI / 2.0) - theta);

		//Getting the minimum allowed acceleration of the axies in motion
                if (gCode[i].p.x != gCode[i + 1].p.x && maxAccel.x < accel)
                        accel = maxAccel.x;
                if (gCode[i].p.y != gCode[i + 1].p.y && maxAccel.y < accel)
                        accel = maxAccel.y;
                if (gCode[i].p.z != gCode[i + 1].p.z && maxAccel.z < accel)
                        accel = maxAccel.y;

                //Finding a radius that will not exceed the max acceleration, assuming the max accel is reached at the worst possible moment for the axis with the lowest allowed acceleration
                //Simplify equation
                arc[i - 1].radius = sqrt (pow (((0.5 * pow (gCode[i + 1].f - gCode[i].f, 2.0)) + ((gCode[i + 1].f - gCode[i].f) * gCode[i].f)) / (arc[i - 1].angle * accel), 2.0) + pow (pow ((gCode[i + 1].f > gCode[i].f)? gCode[i + 1].f : gCode[i].f, 2.0) / accel, 2.0));

                //Finding the time spent traveling on the arc
                arc[i - 1].time = (arc[i - 1].angle * arc[i - 1].radius) / (((gCode[i + 1].f - gCode[i].f) / 2.0) + gCode[i].f);

                //Converting the unit vector to the center to a vector at the center
                centerMag = arc[i - 1].radius / sin (theta);
                arc[i - 1].center = multVec (&toCenter, centerMag);

                //Calculating the arc's start and end points
		length = arc[i - 1].radius / tan (theta);
		line[i].start = multVec (&end, length);
		line[i - 1].end = multVec (&start, length);

		//Calculating a vector from the center to the start point
		arc[i - 1].toSurf = subVec (&line[i - 1].end, &arc[i - 1].center);
		normalize (&arc[i - 1].toSurf);

		//Moving the vectors from 000 to the current G-code position
		arc[i - 1].center = addVec (&arc[i - 1].center, &gCode[i].p);
		line[i].start = addVec (&line[i].start, &gCode[i].p);
		line[i - 1].end = addVec (&line[i - 1].end, &gCode[i].p);

		//Calculating the cross unit vector, needed for the 3D equation of a circle
		normal = crossProduct (&end, &start);
		arc[i - 1].cross = crossProduct (&normal, &arc[i - 1].toSurf);
		normalize (&arc[i - 1].cross);

                //Multiplying the circle equation unit vectors by the radius, needed for the 3D equation of a circle
                arc[i - 1].toSurf = multVec (&arc[i - 1].toSurf, arc[i - 1].radius);
                arc[i - 1].cross = multVec (&arc[i - 1].cross, arc[i - 1].radius);
        }

        //Calculating acceleration, etc of ramp up and ramp down
        getRampInfo (&ramp[0], &gCode[0].p, &gCode[1].p, gCode[0].f, gCode[1].f, &line[0].start);
        getRampInfo (&ramp[1], &gCode[lineNum - 2].p, &gCode[lineNum - 1].p, gCode[lineNum - 1].f, 0.0, &line[lineNum - 2].end);

        //finding the time spent traveling on each line, and a unit vector pointing the direction of travel
        //Make more efficient
        for (i = 0; i < lineNum - 1; i++) {
                line[i].velocity = subVec (&line[i].end, &line[i].start);
                line[i].time = magnitude (&line[i].velocity) / gCode[i + 1].f;
		//printf ("%lf\n", line[i].time);
		normalize (&line[i].velocity);
		line[i].velocity = multVec (&line[i].velocity, gCode[i + 1].f);
        }

	writeFile (ramp, arc, line, &lineNum, argv[1]);

        //Free as soon a possible
        free (gCode);
        free (arc);
        free (line);

        return 0;
}
