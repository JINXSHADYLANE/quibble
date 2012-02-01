#include "utils.h"
#include "memory.h"

SETUP_
{
	rand_init(4516);
}

TEST_(segment_point_dist) {
	Segment s = {{0.0f, 10.0f}, {0.0f, -10.0f}};
	Vector2 p1 = {-7.0f, 3.0f};
	Vector2 p2 = {7.0f, -4.0f};
	Vector2 p3 = {-19.0f, 18.0f};
	Vector2 p4 = {19.0f, -18.0f};

	float d1 = segment_point_dist(s, p1);
	float d2 = segment_point_dist(s, p2);
	float d3 = segment_point_dist(s, p3);
	float d4 = segment_point_dist(s, p4);

	ASSERT_(feql(d1, fabsf(d2)));
	ASSERT_(feql(d3, fabsf(d4)));
}

TEST_(segment_point_dist_adv) {
	Segment s = {{3.0f, 1.0f}, {1.0f, 5.0f}};

	ASSERT_(feql(segment_point_dist(s, vec2(3.0f, 0.0f)), -1.0f));
	ASSERT_(feql(segment_point_dist(s, vec2(4.0f, 1.0f)), 1.0f));
	ASSERT_(feql(segment_point_dist(s, vec2(4.0f, -1.0f)), -sqrtf(5.0f)));
	float d1 = segment_point_dist(s, vec2(1.0f, 2.0f));
	float d2 = segment_point_dist(s, vec2(3.0f, 4.0f));
	ASSERT_(feql(d2, fabsf(d1)));
	ASSERT_(d2 < sqrtf(2.0f) && d2 > 1.0f);
}

TEST_(segment_intersect) {
	Segment s1 = {{1.0f, 5.0f}, {4.0f, 5.0f}};
	Segment s2 = {{4.0f, 5.0f}, {4.0f, 3.0f}};
	Segment s3 = {{4.0f, 2.0f}, {4.0f, 1.0f}};
	Segment s4 = {{2.0f, 2.0f}, {5.0f, 4.0f}};
	Segment s5 = {{3.0f, 2.0f}, {5.0f, 1.0f}};

	Vector2 p;
	ASSERT_(segment_intersect(s1, s2, &p) == true);
	ASSERT_(p.x == 4.0f && p.y == 5.0f);
	ASSERT_(segment_intersect(s2, s3, NULL) == false);
	ASSERT_(segment_intersect(s3, s2, NULL) == false);
	ASSERT_(segment_intersect(s2, s4, NULL) == true);
	ASSERT_(segment_intersect(s4, s2, NULL) == true);
	ASSERT_(segment_intersect(s3, s4, NULL) == false);
	ASSERT_(segment_intersect(s4, s3, NULL) == false);
	ASSERT_(segment_intersect(s3, s5, NULL) == true);
	ASSERT_(segment_intersect(s5, s3, NULL) == true);
	ASSERT_(segment_intersect(s4, s5, NULL) == false);
	ASSERT_(segment_intersect(s5, s4, NULL) == false);
}

TEST_(rand_int)
{
	int i;
	for(i = 0; i < 1000; ++i) {
		int r = rand_int(-11, 8512); 
		ASSERT_(r >= -11 && r <= 8512); 
	}	
}

TEST_(rand_float)
{
	float avg = 0.0f;
	
	int i;
	for(i = 0; i < 50000; ++i) {
		float r = rand_float();
		ASSERT_(r >= 0.0f && r <= 1.0f);
		avg += r;
	}	

	avg = (avg / 50000.0f) - 0.5f;
	ASSERT_(avg >= -0.001f && avg <= 0.001f);
}

TEST_(rand_float_range)
{
	int i;
	for(i = 0; i < 1000; ++i) {
		float r = rand_float_range(-489.0f, 41728.0f);
		ASSERT_(r >= -489.0f && r <= 41728.0f);
	}	
}

TEST_(compression_and_decompression)
{
	char data[1000];
	char* compr_data;
	uint compr_size;

	char* decompr_data;
	uint decompr_size;

	int i;
	for(i = 0; i < 1000; ++i) {
		data[i] = (char)rand_int(0, 10);
	}	

	compr_data = lz_compress(&data[0], 1000, &compr_size);
	ASSERT_(compr_data);
	ASSERT_(compr_size < 1000);

	decompr_data = lz_decompress(compr_data, compr_size, &decompr_size);
	ASSERT_(decompr_data);
	ASSERT_(decompr_size == 1000);

	for(i = 0; i < 1000; ++i) {
		ASSERT_(data[i] == decompr_data[i]);
	}	

	MEM_FREE(compr_data);
	MEM_FREE(decompr_data);
}

bool _close(Color c1, Color c2) {
	uint cycles = 4;
	while(cycles--) {
		int d = (c1 & 0xFF) - (c2 & 0xFF);
		if(abs(d) > 1)
			return false;
		c1 >>= 8; c2 >>= 8;
	}
	return true;
}	

TEST_(rgb_hsv) {
	Color c1 = COLOR_RGBA(0, 0, 0, 0);
	Color c2 = COLOR_RGBA(255, 255, 255, 255);
	Color c3 = COLOR_RGBA(195, 31, 5, 0);
	Color c4 = COLOR_RGBA(95, 249, 111, 0);
	Color c5 = COLOR_RGBA(0, 145, 18, 0);
	Color c6 = COLOR_RGBA(1, 254, 1, 254);

	ASSERT_(_close(hsv_to_rgb(rgb_to_hsv(c1)), c1));
	ASSERT_(_close(hsv_to_rgb(rgb_to_hsv(c2)), c2));
	ASSERT_(_close(hsv_to_rgb(rgb_to_hsv(c3)), c3));
	ASSERT_(_close(hsv_to_rgb(rgb_to_hsv(c4)), c4));
	ASSERT_(_close(hsv_to_rgb(rgb_to_hsv(c5)), c5));
	ASSERT_(_close(hsv_to_rgb(rgb_to_hsv(c6)), c6));
}	

TEST_(endianess) {
	uint16 a = 0x0011;
	uint32 b = 0x00112233;

	a = endian_swap2(a);
	b = endian_swap4(b);

	byte* ba = (byte*)&a;
	ASSERT_(ba[0] == 0x11);
	ASSERT_(ba[1] == 0x00);

	byte* bb = (byte*)&b;
	ASSERT_(bb[0] == 0x33);
	ASSERT_(bb[1] == 0x22);
	ASSERT_(bb[2] == 0x11);
	ASSERT_(bb[3] == 0x00);
}	

TEST_(path_get_folder) {
	ASSERT_(path_get_folder("") == NULL);
	ASSERT_(path_get_folder("long/path/to/") == NULL);

	char* p1 = path_get_folder("long/path/to/file.ext");
	ASSERT_(strcmp(p1, "long/path/to/") == 0);
	MEM_FREE(p1);

	char* p2 = path_get_folder("long/path/to/file");
	ASSERT_(strcmp(p2, "long/path/to/") == 0);
	MEM_FREE(p2);

	char* p4 = path_get_folder("file.extension");
	ASSERT_(strcmp(p4, "./") == 0);
	MEM_FREE(p4);

	char* p5 = path_get_folder("file");
	ASSERT_(strcmp(p5, "./") == 0);
	MEM_FREE(p5);

	char* p6 = path_get_folder("./weird/.path/.file");
	ASSERT_(strcmp(p6, "./weird/.path/") == 0);
	MEM_FREE(p6);

	char* p7 = path_get_folder("windows\\style\\path\\file");
	ASSERT_(strcmp(p7, "windows\\style\\path\\") == 0);
	MEM_FREE(p7);

}	

TEST_(path_change_ext) {
	ASSERT_(path_change_ext("", "ext") == NULL);	
	ASSERT_(path_change_ext("./", "ext") == NULL);	
	ASSERT_(path_change_ext("path/", "ext") == NULL);	
	ASSERT_(path_change_ext("path\\", "ext") == NULL);	
	ASSERT_(path_change_ext("file.txt", ".") == NULL);	
	ASSERT_(path_change_ext("file.txt", ".ext") == NULL);	
	ASSERT_(path_change_ext("file.txt", " ") == NULL);	

	char* p1 = path_change_ext("file.txt", "ext");
	ASSERT_(strcmp(p1, "file.ext") == 0);
	MEM_FREE(p1);

	char* p2 = path_change_ext("file.longext", "");
	ASSERT_(strcmp(p2, "file") == 0);
	MEM_FREE(p2);

	char* p3 = path_change_ext(".file", "ext");
	ASSERT_(strcmp(p3, ".file.ext") == 0);
	MEM_FREE(p3);

	char* p4 = path_change_ext("path/.file", "ext");
	ASSERT_(strcmp(p4, "path/.file.ext") == 0);
	MEM_FREE(p4);
	
	char* p5 = path_change_ext("path/.file", "");
	ASSERT_(strcmp(p5, "path/.file") == 0);
	MEM_FREE(p5);

	char* p6 = path_change_ext("path/.to/file", "longext");
	ASSERT_(strcmp(p6, "path/.to/file.longext") == 0);
	MEM_FREE(p6);

	char* p7 = path_change_ext("path/to/file.tar.gz", "bz");
	ASSERT_(strcmp(p7, "path/to/file.tar.bz") == 0)
	MEM_FREE(p7);
}

TEST_(is_pow2) {
	uint positive[] = {1, 2, 4, 8, 1024, 64, 262144, 2147483648};
	uint negative[] = {0, 3, 7, 255, 4097, 458290, 9851754, 4294967295};
	
	for(uint i = 0; i < ARRAY_SIZE(positive); ++i) {
		ASSERT_(is_pow2(positive[i]) == true);
	}	

	for(uint i = 0; i < ARRAY_SIZE(negative); ++i) {
		ASSERT_(is_pow2(negative[i]) == false);
	}	
}

TEST_(base64) {
	byte tests[6][9] = {
		{42, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0,  0, 0, 0, 0, 0, 0, 0},
		{1, 0, 8,  0, 0, 0, 0, 0, 0},
		{3, 1, 5, 7,  0, 0, 0, 0, 0},
		{9, 8, 6, 5, 3,  0, 0, 0, 0}, 
		{1, 1, 2, 3, 5, 8, 13, 21, 34}
	};

	for(uint i = 0; i < 6; ++i) {
		uint size = i < 5 ? i + 1 : 9;
		uint encoded_size;
		char* encoded = base64_encode(tests[i], size, &encoded_size);
		
		uint decoded_size;
		byte* decoded = (byte*)base64_decode(encoded, encoded_size, &decoded_size);
		ASSERT_(decoded_size == size);

		for(uint j = 0; j < size; ++j) {
			ASSERT_(tests[i][j] == decoded[j]);
		}

		MEM_FREE(encoded);
		MEM_FREE(decoded);
	}
}

TEST_(rectf_cut) {
	RectF a, b;
	RectF out[4];

	// b fully inside a
	a = rectf(0.0f, 0.0f, 10.0f, 10.0f);
	b = rectf(1.0f, 1.0f, 9.0f, 9.0f);
	ASSERT_(rectf_cut(&a, &b, out) == 4);	
	ASSERT_(out[0].left == 0.0f && out[0].top == 0.0f && out[0].right == 1.0f && out[0].bottom == 10.0f);
	ASSERT_(out[1].left == 1.0f && out[1].top == 0.0f && out[1].right == 9.0f && out[1].bottom == 1.0f);
	ASSERT_(out[2].left == 1.0f && out[2].top == 9.0f && out[2].right == 9.0f && out[2].bottom == 10.0f);
	ASSERT_(out[3].left == 9.0f && out[3].top == 0.0f && out[3].right == 10.0f && out[3].bottom == 10.0f);

	// no intersection
	b = rectf(10.0f, 0.0f, 20.0f, 10.0f);
	ASSERT_(rectf_cut(&a, &b, out) == 1);
	ASSERT_(out[0].left == a.left && out[0].top == a.top && out[0].right == a.right && out[0].bottom == a.bottom);

	// partial horizontal intersection
	b = rectf(-5.0f, 5.0f, 5.0f, 7.0f);
	ASSERT_(rectf_cut(&a, &b, out) == 3);
	ASSERT_(out[0].left == 0.0f && out[0].top == 0.0f && out[0].right == 5.0f && out[0].bottom == 5.0f);
	ASSERT_(out[1].left == 0.0f && out[1].top == 7.0f && out[1].right == 5.0f && out[1].bottom == 10.0f);
	ASSERT_(out[2].left == 5.0f && out[2].top == 0.0f && out[2].right == 10.0f && out[2].bottom == 10.0f);

	// partial vertical intersection
	b = rectf(2.0f, -2.0f, 8.0f, 2.0f);
	ASSERT_(rectf_cut(&a, &b, out) == 3);
	ASSERT_(out[0].left == 0.0f && out[0].top == 0.0f && out[0].right == 2.0f && out[0].bottom == 10.0f);
	ASSERT_(out[1].left == 2.0f && out[1].top == 2.0f && out[1].right == 8.0f && out[1].bottom == 10.0f);
	ASSERT_(out[2].left == 8.0f && out[2].top == 0.0f && out[2].right == 10.0f && out[2].bottom == 10.0f);

	// corner intersection
	b = rectf(-1.0f, -1.0f, 1.0f, 1.0f);
	ASSERT_(rectf_cut(&a, &b, out) == 2);
	ASSERT_(out[0].left == 0.0f && out[0].top == 1.0f && out[0].right == 1.0f && out[0].bottom == 10.0f);
	ASSERT_(out[1].left == 1.0f && out[1].top == 0.0f && out[1].right == 10.0f && out[1].bottom == 10.0f);
}

TEST_(rectf_inside_circle) {
	RectF a = {-1.0f, -1.0f, 1.0f, 1.0f};
	RectF b = {2.0f, 2.0f, 4.0f, 6.0f};

	Vector2 ca = {0.0f, 0.0f};
	Vector2 cb = {1.0f, 4.0f};

	ASSERT_(rectf_inside_circle(&a, &ca, 1.0f) == false);
	ASSERT_(rectf_inside_circle(&b, &ca, 1.0f) == false);
	ASSERT_(rectf_inside_circle(&a, &ca, sqrtf(2.0f) + 0.1f) == true);
	ASSERT_(rectf_inside_circle(&a, &ca, sqrtf(2.0f) - 0.1f) == false);
	ASSERT_(rectf_inside_circle(&b, &ca, sqrtf(2.0f) + 0.1f) == false);
	ASSERT_(rectf_inside_circle(&a, &cb, 3.0f) == false);
	ASSERT_(rectf_inside_circle(&b, &cb, 3.0f) == false);
	ASSERT_(rectf_inside_circle(&a, &cb, sqrtf(13.0f) + 0.1f) == false);
	ASSERT_(rectf_inside_circle(&b, &cb, sqrtf(13.0f) + 0.1f) == true);
	ASSERT_(rectf_inside_circle(&b, &cb, sqrtf(13.0f) - 0.1f) == false);
	ASSERT_(rectf_inside_circle(&a, &cb, 7.0f) == true);
	ASSERT_(rectf_inside_circle(&b, &cb, 7.0f) == true);
}

bool _vec2_feql(Vector2 v, float x, float y) {
	return feql(v.x, x) && feql(v.y, y); 
}

TEST_(circle_raycast) {
	Vector2 ca = {1.0f, 1.0f};
	float ra = 1.0f;
	Vector2 cb = {2.0f, 3.0f};
	float rb = 1.5f;

	Vector2 sa[] = {{0.0f, 2.0f}, {3.0f, 2.0f}};
	Vector2 sb[] = {{1.0f, 1.0f}, {2.0f, 3.0f}};
	Vector2 sc[] = {{7.0f, 0.0f}, {5.0f, 1.0f}};
	Vector2 sd[] = {{4.0f, 5.0f}, {1.0f, 0.0f}};
	Vector2 se[] = {{1.0f, 5.0f}, {3.0f, 6.0f}};

	ASSERT_(_vec2_feql(circle_raycast(&ca, ra, &sa[0], &sa[1]), 1.0f, 2.0f));
	ASSERT_(_vec2_feql(circle_raycast(&cb, rb, &sa[0], &sa[1]), 0.88196f, 2.0f));
	
	ASSERT_(_vec2_feql(circle_raycast(&ca, ra, &sb[0], &sb[1]), sb[0].x, sb[0].y));

	ASSERT_(_vec2_feql(circle_raycast(&ca, ra, &sc[0], &sc[1]), sc[1].x, sc[1].y));
	ASSERT_(_vec2_feql(circle_raycast(&cb, rb, &sc[0], &sc[1]), sc[1].x, sc[1].y));

	ASSERT_(!_vec2_feql(circle_raycast(&ca, ra, &sd[0], &sd[1]), sd[1].x, sd[1].y));
	ASSERT_(!_vec2_feql(circle_raycast(&cb, rb, &sd[0], &sd[1]), sd[1].x, sd[1].y));

	ASSERT_(_vec2_feql(circle_raycast(&ca, ra, &se[0], &se[1]), se[1].x, se[1].y));
	ASSERT_(_vec2_feql(circle_raycast(&cb, rb, &se[0], &se[1]), se[1].x, se[1].y))
}

TEST_(rectf_raycast) {
	RectF r = {-10.1f, 3.2f, 4.0f, 18.41f};

	Vector2 s = {-20.0f, 5.0f};
	Vector2 e = {20.0f, 5.0f};
	Vector2 h = rectf_raycast(&r, &s, &e);
	ASSERT_(feql(h.x, -10.1f) && h.y == s.y);

	h = rectf_raycast(&r, &e, &s);
	ASSERT_(feql(h.x, 4.0f) && h.y == s.y);

	s = vec2(0.2f, 0.0f);
	e = vec2(0.2f, 20.0f);
	h = rectf_raycast(&r, &s, &e);
	ASSERT_(h.x == s.x && feql(h.y, 3.2f));

	h = rectf_raycast(&r, &e, &s);
	ASSERT_(h.x == s.x && feql(h.y, 18.41f));
}

