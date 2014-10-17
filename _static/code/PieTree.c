/* copyright 2007 Walter Brisken
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* a Makefile:
 *
 * CFLAGS = -Wall -W -O2 -g `pkg-config --cflags cairo`
 * LIBS = `pkg-config --libs cairo`
 * 
 * PieTree: PieTree.o
 *         gcc PieTree.o -o PieTree $(LIBS)
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <cairo.h>
#include <cairo-pdf.h>

const char program[] = "PieTree";
const char version[] = "0.1";
const char verdate[] = "2007/12/21";
const char author[]  = "Walter Brisken";
const char authoremail[] = "wbrisken@nrao.edu";

struct config
{
	double pieradius;
	double boxsize;
	double xmax;
	double xmargin, ymargin;
	double xscale, tipspacing;
	double linethick;
	double rimthick;
	double radiusthick;
	double rgb0[3], rgb1[3];
	double tipnamesize;
	double nodenamesize;
	double colorthreshhold;
};

struct node
{
	struct node *left, *right;
	char name[100];
	double x, y;
	double trait;
	double age;
};

struct config *newConfig()
{
	struct config *c;

	c = (struct config *)calloc(1, sizeof(struct config));

	return c;
}

void deleteConfig(struct config *c)
{
	if(c)
	{
		free(c);
	}
}

struct node *newNode()
{
	struct node *n;

	n = (struct node *)calloc(1, sizeof(struct node));

	return n;
}

void deleteTree(struct node *n)
{
	if(n)
	{
		if(n->left)
		{
			deleteTree(n->left);
		}
		if(n->right)
		{
			deleteTree(n->right);
		}
		free(n);
	}
}

int findcomma(const char *str)
{
	int p = 0;
	int i;

	for(i = 0; str[i]; i++)
	{
		switch(str[i])
		{
		case '(': 
			p++; 
			break;
		case ')': 
			p--; 
			break;
		case ',': 
			if(!p) 
			{
				return i;
			}
			break;
		}
	}

	return -10;
}

/* warning -- ruins str */
void parseElement(char *str, struct node *n)
{
	int i, v;

	for(i = 0; str[i]; i++)
	{
		if(str[i] == ':')
		{
			break;
		}
	}
	
	if(str[i])
	{
		str[i] = 0;
		v = sscanf(str+i+1, "%lf", &n->age);
		if(v != 1)
		{
			fprintf(stderr, "ERROR : %s\n", str);
		}
	}

	strcpy(n->name, str);
}

/* warning -- ruins str */
struct node *parseTree(char *str)
{
	struct node *n;
	int l, i=-1, s=0, p;

	l = strlen(str);
	while(str[l-1] == ';' || str[l-1] < ' ')
	{
		l--;
		str[l] = 0;
	}

	n = newNode();

	if(str[0] == '(')
	{
		for(i = l-1; i; i--)
		if(str[i] == ')')
		{
			break;
		}
		if(i)
		{
			do
			{
				str[s] = 0;
				str[i-s] = 0;
				s++;
				p = findcomma(str+s)+s;
				if(s >= l/2)
				{
					fprintf(stderr, "Trouble parsing: %s\n", str+s);
					deleteTree(n);
					return 0;
				}
			}
			while(p < 0);
			str[p] = 0;
			n->left = parseTree(str+s);
			n->right = parseTree(str+p+1);
			if(n->left == 0 || n->right == 0)
			{
				deleteTree(n);
				return 0;
			}
		}
	}
	parseElement(str+i+1, n);

	return n;
}

struct node *findNode(struct node *n, const char *name)
{
	struct node *l;

	if(!n)
	{
		return 0;
	}

	if(strcmp(n->name, name) == 0)
	{
		return n;
	}
	l = findNode(n->left, name);
	return l?l:findNode(n->right, name);
}

void parseTrait(struct node *tree, const char *str)
{
	int v;
	struct node *n;
	char name[100], tstr[100];

	if(sscanf(str, "%s%s", name, tstr) < 2)
	{
		fprintf(stderr, "ERROR : %s\n", str);
		return;
	}

	n = findNode(tree, name);
	if(n)
	{
		v = sscanf(tstr, "%lf", &n->trait);
		if(v != 1)
		{
			fprintf(stderr, "ERROR : %s is not a number\n", tstr);
		}
	}
	else
	{
		fprintf(stderr, "ERROR : %s is not a node\n", name);
	}
}

void printTree(const struct node *n, int indent)
{
	int i;

	if(!n)
	{
		return;
	}
	for(i = 0; i < indent; i++) 
	{
		putchar(' ');
	}
	printf("%s : age=%f trait=%f x = %f y = %f\n", 
		n->name, n->age, n->trait, n->x, n->y);
	printTree(n->left, indent+2);
	printTree(n->right, indent+2);
}

int genlength(struct node *tree, const struct config *c, int m)
{
	int D, r, l;

	if(tree->left == 0 || tree->right == 0)
	{
		D = m;
	}
	else
	{
		l = genlength(tree->left, c, m+1);
		r = genlength(tree->right, c, m+1);
		D = l>r ? l : r;
	}

	tree->x = m*c->xmax/D;

	return D;
}

int calcXY(struct node *tree, double x, int i, double *maxx)
{
	if(tree == 0)
	{
		return i;
	}
	x += tree->age;
	tree->x = x;
	if(x > *maxx)
	{
		*maxx = x;
	}
	i = calcXY(tree->left, x, i, maxx);
	i = calcXY(tree->right, x, i, maxx);
	if(tree->left == 0 || tree->right == 0)
	{
		tree->y = i;
		i++;
		return i;
	}
	else
	{
		tree->y = (tree->left->y + tree->right->y)*0.5;
	}

	return i;
}

struct node *loadTree(const char *filename)
{
	struct node *tree;
	char line[10000];
	FILE *in;

	in = fopen(filename, "r");
	if(!in)
	{
		fprintf(stderr, "ERROR : file %s doesn't exist\n",
			filename);
		return 0;
	}
	fgets(line, 9999, in);
	if(feof(in))
	{
		fclose(in);
		return 0;
	}

	tree = parseTree(line);

	for(;;)
	{
		fgets(line, 9999, in);
		if(feof(in))
		{
			fclose(in);
			return tree;
		}
		parseTrait(tree, line);
	}

	printTree(tree, 4);

	return tree;
}

void xform(double x_in, double y_in, const struct config *c, double *x_out, double *y_out)
{
	*x_out = c->xmargin + x_in*c->xscale;
	*y_out = c->ymargin + y_in*c->tipspacing;
}

void getrgb(const struct config *c, const struct node *n, double *r, double *g, double *b)
{
	*r = c->rgb0[0]*(1.0-n->trait) + c->rgb1[0]*n->trait;
	*g = c->rgb0[1]*(1.0-n->trait) + c->rgb1[1]*n->trait;
	*b = c->rgb0[2]*(1.0-n->trait) + c->rgb1[2]*n->trait;
}

void drawTip(cairo_t *cr, const struct node *tree, const struct config *c)
{
	double x, y, delta;
	double r, g, b;
	cairo_text_extents_t extents;

	getrgb(c, tree, &r, &g, &b);
	delta = c->boxsize;
	xform(tree->x, tree->y, c, &x, &y);
	cairo_rectangle(cr, x-0.5*delta, y-0.5*delta, delta, delta);
	cairo_set_source_rgb(cr, r, g, b);
	cairo_fill(cr);
	if(c->rimthick > 0.0)
	{
		cairo_set_line_width(cr, c->rimthick);
		cairo_rectangle(cr, x-0.5*delta, y-0.5*delta, delta, delta);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_stroke(cr);
	}

	if(c->tipnamesize > 0.0)
	{
		cairo_set_font_size(cr, c->tipnamesize);
		cairo_text_extents(cr, tree->name, &extents);
		cairo_move_to(cr, x+delta, y+extents.height*0.5);
		cairo_show_text(cr, tree->name);
	}
}

double maxtipnamesize(cairo_t *cr, const struct node *tree, const struct config *c, double size)
{	
	cairo_text_extents_t extents;

	if(tree)
	{
		if(tree->left == 0 || tree->right == 0)
		{
			if(size < 0.0)
			{
				cairo_set_font_size(cr, c->tipnamesize);
			}
			cairo_text_extents(cr, tree->name, &extents);
			if(extents.width > size)
			{
				size = extents.width;
			}
		}
		else
		{
			size = maxtipnamesize(cr, tree->left, c, size);
			size = maxtipnamesize(cr, tree->right, c, size);
		}
	}

	return size;
}

int countTips(const struct node *tree)
{
	if(!tree)
	{
		return 0;
	}
	if(tree->left && tree->right)
	{
		return countTips(tree->left) + countTips(tree->right);
	}
	else
	{
		return 1;
	}
}

void drawPie(cairo_t *cr, const struct node *tree, const struct config *c)
{
	double x, y, R;
	double angle;

	xform(tree->x, tree->y, c, &x, &y);

	angle = 2.0*M_PI*(1.0-tree->trait);
	R = c->pieradius;

	if(c->radiusthick > 0.0)
	{
		cairo_set_line_width(cr, c->radiusthick);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_move_to(cr, x, y);
		cairo_line_to(cr, x, y-R);
		cairo_stroke(cr);
		cairo_move_to(cr, x+R*sin(angle), y-R*cos(angle));
		cairo_stroke(cr);
	}

	cairo_move_to(cr, x, y);
	cairo_line_to(cr, x, y-R);
	cairo_set_source_rgb(cr, c->rgb0[0], c->rgb0[1], c->rgb0[2]);
	cairo_arc(cr, x, y, R, -M_PI/2.0, -M_PI/2 + angle);
	cairo_close_path(cr);
	cairo_fill(cr);

	cairo_move_to(cr, x, y);
	cairo_line_to(cr, x, y-R);
	cairo_set_source_rgb(cr, c->rgb1[0], c->rgb1[1], c->rgb1[2]);
	cairo_arc_negative(cr, x, y, R, -M_PI/2.0, -M_PI/2 + 2.0*M_PI*(1.0-tree->trait));
	cairo_close_path(cr);
	cairo_fill(cr);

	if(c->rimthick > 0.0)
	{
		cairo_set_line_width(cr, c->rimthick);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_arc(cr, x, y, R, 0, 2*M_PI);
		cairo_stroke(cr);
	}
}

void drawVees(cairo_t *cr, const struct node *tree, const struct config *c)
{
	double x, y, xl, yl, xr, yr;

	xform(tree->x,        tree->y,        c, &x,  &y);
	xform(tree->left->x,  tree->left->y,  c, &xl, &yl);
	xform(tree->right->x, tree->right->y, c, &xr, &yr);

	cairo_set_line_width(cr, c->linethick);
	cairo_move_to(cr, x, y);
	cairo_line_to(cr, xl, yl);
	cairo_stroke(cr);
	cairo_move_to(cr, x, y);
	cairo_line_to(cr, xr, yr);
	cairo_stroke(cr);
}

void drawFork(cairo_t *cr, const struct node *tree, const struct config *c)
{
	double x, y, xl, yl, xr, yr;

	xform(tree->x,        tree->y,        c, &x,  &y);
	xform(tree->left->x,  tree->left->y,  c, &xl, &yl);
	xform(tree->right->x, tree->right->y, c, &xr, &yr);

	cairo_set_line_width(cr, c->linethick);
	cairo_move_to(cr, x, y);
	cairo_line_to(cr, x, yl);
	cairo_line_to(cr, xl, yl);
	cairo_stroke(cr);
	cairo_move_to(cr, x, y);
	cairo_line_to(cr, x, yr);
	cairo_line_to(cr, xr, yr);
	cairo_stroke(cr);
}

void drawNodeLabel(cairo_t *cr, const struct node *tree, const struct config *c)
{
	cairo_text_extents_t extents;
	double x, y;

	xform(tree->x, tree->y, c, &x, &y);

	if(c->nodenamesize > 0.0)
	{
		cairo_set_font_size(cr, c->nodenamesize);
		cairo_text_extents(cr, tree->name, &extents);
		cairo_move_to(cr, x+c->boxsize*0.5+c->pieradius, y+extents.height*0.5);
		cairo_show_text(cr, tree->name);
	}
}

void plotTree(cairo_t *cr, const struct node *tree, const struct config *c)
{
	if(tree->left == 0 || tree->right == 0)
	{
		drawTip(cr, tree, c);
	}
	else
	{
		drawFork(cr, tree, c);
		drawPie(cr, tree, c);
		drawNodeLabel(cr, tree, c);
	
		plotTree(cr, tree->left, c);
		plotTree(cr, tree->right, c);
	}
}

void drawRoot(cairo_t *cr, const struct node *tree, const struct config *c)
{
	double x, y, x0;

	xform(0.0, tree->y,     c, &x0, &y);
	xform(tree->x, tree->y, c, &x,  &y);
	
	cairo_set_line_width(cr, c->linethick);
	cairo_move_to(cr, x, y);
	cairo_line_to(cr, x0, y);
	cairo_stroke(cr);
}

int usage()
{
	printf("\n%s ver. %s  %s  %s <%s>\n", program, version, author, verdate, authoremail);
	printf("\nusage:  %s <input tree> <output pdf>\n\n", program);

	return 1;
}

int main(int argc, char **argv)
{
	struct node *tree;
	struct config *c;
	cairo_surface_t *surface;
	cairo_t *cr;
	unsigned char *image;
	double tipsize;
	int width = 6*72;
	int height;
	int numtips;

	if(argc < 3)
	{
		return usage();
	}

	c = newConfig();

	c->pieradius = 3;
	c->boxsize = 4.8;
	c->tipspacing = c->boxsize*1.5;
	c->xmargin = 6.0;
	c->ymargin = 6.0;
	c->rgb0[0] = 0.8;
	c->rgb0[1] = 0.4;
	c->rgb0[2] = 0.0;
	c->rgb1[0] = 0.0;
	c->rgb1[1] = 0.45;
	c->rgb1[2] = 0.7;
	c->rimthick = .5;
	c->radiusthick = .5;
	c->linethick = .8;
	c->tipnamesize = 4.5;
	c->nodenamesize = 3;

	tree = loadTree(argv[1]);
	calcXY(tree, 0.0, 0, &c->xmax);

	genlength(tree, c, 1);

	numtips = countTips(tree);
	height = (numtips+1)*c->tipspacing;

	if(0)
	{
		image = (unsigned char *)malloc(width*height*4);
		surface = cairo_image_surface_create_for_data(
			image, CAIRO_FORMAT_ARGB32, width, height, 4*width);
	}
	else
	{
		surface = cairo_pdf_surface_create(argv[2], 6*72, height);
	}
	cr = cairo_create (surface);

	tipsize = maxtipnamesize(cr, tree, c, -1.0);
	c->xscale = (width-2*c->xmargin-c->boxsize-tipsize)/c->xmax;
	printf("ts = %f\n", tipsize);

	drawRoot(cr, tree, c);
	plotTree(cr, tree, c);
	
	if(0)
	{
		cairo_surface_write_to_png (surface, "tree.png");
	}
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	deleteTree(tree);
	deleteConfig(c);

	return 0;
}
