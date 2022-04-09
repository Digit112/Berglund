#include <math.h>
#include <stdio.h>
#include <stdlib.h>

# include <cstdint>

// Represents a circle as a position of origin and radius
class circle {
public:
	double r;
	double x;
	double y;
	
	circle() : x(0), y(0), r(1) {}
	circle(double x, double y, double r) : x(x), y(y), r(r) {}
};

// Allows us to handle vectors as individual objects
class vec {
public:
	double x;
	double y;
	
	vec(double x, double y) : x(x), y(y) {}
};

// Includes point of collision and direction of new ray.
class collision {
public:
	bool does_collide;
	
	// Point at which collision occured.
	vec coll_pnt;
	
	// Vector representing the direction of the new ray.
	vec coll_dir;
	
	collision(bool dc, double cpx, double cpy, double cdx, double cdy) : does_collide(dc), coll_pnt(cpx, cpy), coll_dir(cdx, cdy) {}
};

// RGB image
class img {
public:
	int width;
	int height;
	int size;
	
	uint8_t* data;
	
	// Create an image with the given width and height. Image size cannot be changed after creation.
	// Image will contain uninitialized data.
	img(int width, int height) : width(width), height(height), size(width*height), data((uint8_t*) malloc(width*height*3)) {}
	
	// Set all pixels in the image to the passed color.
	void fill(uint8_t r, uint8_t g, uint8_t b) {
		for (int i = 0; i < size; i++) {
			data[i*3  ] = r;
			data[i*3+1] = g;
			data[i*3+2] = b;
		}
	}
	
	// Saves as 3-channel bytes (P6) PGM image
	void save(const char* fn) {
		char hdr[20];
		int hdr_len = sprintf(hdr, "P6 %d %d 255 ", width, height);
		
		FILE* fout = fopen(fn, "wb");
		fwrite(hdr, 1, hdr_len, fout);
		fwrite(data, 1, size*3, fout);
		fclose(fout);
	}
	
	~img() {
		free(data);
	}
};

// Renders the listed circles to the passed img.
void render_circles(img& im, circle* circles, int circles_n, double mx, double my, double Mx, double My) {
	for (int c_i = 0; c_i < circles_n; c_i++) {
		circle& c = circles[c_i];
		
		int px = (c.x - mx) / (Mx - mx) * im.width;
		int py = (c.y - my) / (My - my) * im.height;
		int pr = im.width / (Mx - mx) * c.r;
		
		int max_i = (int) (1.5*2*M_PI*pr);
		for (int i = 0; i < max_i; i++) {
			double t = (double) i/max_i * 2*M_PI;
			int dx = px + pr*cos(t);
			int dy = py + pr*sin(t);
			
			if (dx >= 0 and dx < im.width and dy >= 0 and dy < im.height) {
				im.data[(dx + dy*im.width)*3  ] = 255;
				im.data[(dx + dy*im.width)*3+1] = 255;
				im.data[(dx + dy*im.width)*3+2] = 255;
			}
		}
	}
}

void render_point(img& im, double x, double y, uint8_t r, uint8_t g, uint8_t b) {
	for (int a = x-1; a <= x+1; a++) {
		for (int b = y-1; b <= y+1; b++) {
			if (a >= 0 && a < im.width && b >= 0 && b < im.height) {
				im.data[(a + b*im.width)*3  ] = r;
				im.data[(a + b*im.width)*3+1] = g;
				im.data[(a + b*im.width)*3+2] = b;
			}
		}
	}
}

void render_line(img& im, double x1, double y1, double x2, double y2, uint8_t r, uint8_t g, uint8_t b) {
	int dx = x2-x1;
	int dy = y2-y1;
	int max_i = (int) (1.1*sqrt(dx*dx+dy*dy));
	for (int i = 0; i < max_i; i++) {
		int px = (double) i / max_i * dx + x1;
		int py = (double) i / max_i * dy + y1;
		
		if (px < 0 || px >= im.width || py < 0 || py >= im.height) {
			continue;
		}
		
		im.data[(px + py*im.width)*3  ] = r;
		im.data[(px + py*im.width)*3+1] = g;
		im.data[(px + py*im.width)*3+2] = b;
	}
}

double sqr_dis(vec a, vec b) {
	double dx = a.x - b.x;
	double dy = a.y - b.y;
	return dx*dx + dy*dy;
}

// Collide a point with a circle. Returns true if the point is in the circle, flase otherwise.
bool collide_point(circle& c, double x, double y) {
	double dx = x - c.x;
	double dy = y - c.y;
	return dx*dx + dy*dy <= c.r*c.r;
}

// Test if a point is inside a rectangle. This code is devolving rapidly.
bool collide_point_rect(double x, double y, double x1, double y1, double x2, double y2) {
	if (x1 > x2) {
		double temp = x1;
		x1 = x2;
		x2 = temp;
	}
	if (y1 > y2) {
		double temp = y1;
		y1 = y2;
		y2 = temp;
	}
	
	return x >= x1 && x <= x2 && y >= y1 && y <= y2;
}

// Collides a line segment defined by a starting and ending position with a circle and returns a collision object.
collision collide_line(circle& c, double x1, double y1, double x2, double y2) {
	double dx = x1 - x2;
	double dy = y1 - y2;
	double len = sqrt(dx*dx + dy*dy);
	
	double dot = (((c.x-x1)*(x2-x1)) + ((c.y-y1)*(y2-y1))) / (len*len);
	
	double fx = x1 + (dot * (x2-x1));
	double fy = y1 + (dot * (y2-y1));
	
	// If either of the endpoints of the line collide with the circle or the closest point collides with the circle and is within
	// The line segment's bounding box, the line and circle are colliding.
	if (collide_point(c, x1, y1) || collide_point(c, x2, y2) || (collide_point(c, fx, fy) && collide_point_rect(fx, fy, x1, y1, x2, y2))) {
		// Get the distance from (fx, fy) to the circles origin.
		double dis = sqrt((fx-c.x)*(fx-c.x) + (fy-c.y)*(fy-c.y));
		
		// Calculate the distance along the line segment from the closest point to the circle's origin (fx, fy) to the circle's edge.
		double travel = sqrt(c.r*c.r - dis*dis);
		
		// Get a vector pointing from (fx, fy) to (x1, y1) (the ray's origin)
		vec rev(x1-fx, y1-fy);
		
		// Normalize this vector to be of travel length.
		double mag = sqrt(rev.x*rev.x + rev.y*rev.y);
		
		mag /= travel;
		
		rev.x /= mag;
		rev.y /= mag;
		
		// The point of collision.
		vec c_pnt(rev.x + fx, rev.y + fy);
		
		// Get the normal vector and normalize.
		vec n((c_pnt.x - c.x)/c.r, (c_pnt.y - c.y)/c.r);
		
		// Get the direction vector of the ray,
		vec d(x2 - x1, y2 - y1);
		
		// and normalize.
		mag = sqrt(d.x*d.x + d.y*d.y);
		d.x /= mag;
		d.y /= mag;
		
		dot = d.x*n.x + d.y*n.y;
		n.x *= 2*dot;
		n.y *= 2*dot;
		
		vec c_dir(d.x - n.x, d.y - n.y);
		
		return collision(true, c_pnt.x, c_pnt.y, c_dir.x, c_dir.y);
	}
	
	return collision(false, 0, 0, 0, 0);
}

int main() {
	circle circles[19];
	for (int i = 1; i < 7; i++) {
		double t = 2*M_PI * ((double) (i-1)/6);
		circles[i].x = 3*cos(t);
		circles[i].y = 3*sin(t);
	}
	for (int i = 7; i < 19; i++) {
		double t = 2*M_PI * ((double) (i-7)/12);
		circles[i].x = 6*cos(t);
		circles[i].y = 6*sin(t);
	}
	
	int width = 1920;
	int height = 1080;
	
	double mx = -14.22;
	double my = -8;
	double Mx = 14.22;
	double My = 8;
	
	img out(width, height);
	
	for (int i = 0; i < 96; i++) {
		double x1 = 7;
		double y1 = 7 + (double) i / 96 * 0.0000005;
		double x2 = -3;
		double y2 = -4;
		
		out.fill(0, 0, 0);
		render_circles(out, circles, 19,  mx, my, Mx, My);
		
		int limit = 10000;
		int last_col = -1;
		while (limit > 0) {
			limit--;
			
			int col_num = -1;
			collision mcl = collision(false, 0, 0, 0, 0);
			
			for (int i = 0; i < 19; i++) {
				if (i == last_col) {
					continue;
				}
				
				collision cl = collide_line(circles[i], x1, y1, x2, y2);
				
				if (cl.does_collide && (!mcl.does_collide || sqr_dis(vec(x1, y1), cl.coll_pnt) < sqr_dis(vec(x1, y1), mcl.coll_pnt))) {
					mcl = cl;
					col_num = i;
				}
			}
			
			last_col = col_num;
			
//			render_point(out, mcl.coll_pnt.x, mcl.coll_pnt.y, 0, 0, 255);
			
			int px1 = (x1-mx) / (Mx-mx) * width;
			int py1 = (y1-my) / (My-my) * height;
			
			if (!mcl.does_collide) {
				int px2 = (x2-mx) / (Mx-mx) * width;
				int py2 = (y2-my) / (My-my) * height;
				render_line(out, px1, py1, px2, py2, 255, 0, 0);
				break;
			}
			
			int px2 = (mcl.coll_pnt.x-mx) / (Mx-mx) * width;
			int py2 = (mcl.coll_pnt.y-my) / (My-my) * height;
			render_line(out, px1, py1, px2, py2, 255, 0, 0);
			
			x1 = mcl.coll_pnt.x;
			y1 = mcl.coll_pnt.y;
			
			double mag = sqrt(mcl.coll_dir.x*mcl.coll_dir.x + mcl.coll_dir.y*mcl.coll_dir.y);
			mag /= 12;
			x2 = x1 + mcl.coll_dir.x/mag;
			y2 = y1 + mcl.coll_dir.y/mag;
		}
		
		char fn[16];
		sprintf(fn, "out/%03d.pgm", i);
		out.save(fn);
	}
	
	return 0;
}