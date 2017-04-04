/*
Copyright (c) 2016-2017 Jorge Giner Cordero

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* Enigma M4 Cipher Machine Emulator */

#include "config.h"
#include "ngetopt.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define NELEMS(a) (sizeof(a)/sizeof(a[0]))

#define TOKSEP " \r\n"

enum {
	N = 26,
	MACHINE_NROTORS = 5,
};

enum {
	ROTOR_1,
	ROTOR_2,
	ROTOR_3,
	ROTOR_4,
	ROTOR_5,
	ROTOR_6,
	ROTOR_7,
	ROTOR_8,
	ROTOR_BETA,
	ROTOR_GAMMA,
	ROTOR_B,
	ROTOR_C,
	NROTORS
};

struct rotor {
	const char *name;
	const char *alphabet;
	const char *stepwhen;
	unsigned char ring;
	unsigned char wires[N];
	unsigned char rwires[N];
};

struct plugboard {
	unsigned char wires[N];
};

struct machine {
	unsigned char rotors[MACHINE_NROTORS];
	unsigned char base[MACHINE_NROTORS];
	struct plugboard pboard;
};

static struct rotor s_rotors[NROTORS] = {
	{ "I",     "EKMFLGDQVZNTOWYHXUSPAIBRCJ", "Q" },
	{ "II",    "AJDKSIRUXBLHWTMCQGZNPYFVOE", "E" },
	{ "III",   "BDFHJLCPRTXVZNYEIWGAKMUSQO", "V" },
	{ "IV",    "ESOVPZJAYQUIRHXLNFTGKDCMWB", "J" },
	{ "V",     "VZBRGITYUPSDNHLXAWMJQOFECK", "Z" },
	{ "VI",    "JPGVOUMFYQBENHZRDKASXLICTW", "ZM" },
	{ "VII",   "NZJHGRCXMYSWBOUFAIVLPEKQDT", "ZM" },
	{ "VIII",  "FKQHTLXOCBJSPDZRAMEWNIUYGV", "ZM" },
	{ "Beta",  "LEYJVCNIXWPBQMDRTAKZGFUHOS", "" },
       	{ "Gamma", "FSOKANUERHMBTIYCWLQPZXVGJD", "" },
	{ "b",     "ENKQAUYWJICOPBLMDXZVFTHRGS", "" },
	{ "c",     "RDOBJNTKVEHMLFCWZAXGYIPSUQ", "" },
};

static struct machine s_machine = {
	{ ROTOR_B, ROTOR_BETA, ROTOR_1, ROTOR_2, ROTOR_3 },
	{ 0, 0, 0, 0, 0 }
};

enum {
	MODE_LOOP,
	MODE_FILTER,
	MODE_FILE
};

static int s_mode = MODE_LOOP;
static int s_debug_on = 0;
static char *s_srcfname = NULL;
static char *s_dstfname = NULL;

/* Case insensitive compare. Returns 1 or 0. */
static int streq(const char *a, const char *b)
{
	while (*a != '\0' && *b != '\0') {
		if (toupper(*a) != toupper(*b))
			return 0;
		a++;
		b++;
	}
	return *a == *b;
}

/* Maps a-z to 0-25. The same for A-Z. Returns -1 otherwise. */
static int tonum(int c)
{
	if (c >= 'a' && c <= 'z')
		return c - 'a';
	else if (c >= 'A' && c <= 'Z')
		return c - 'A';
	else
		return -1;
}

/* Checks that the wires are complete, i.e., they map the 26 codes. */
static int are_wires_ok(unsigned char *wires)
{
	int i;
	unsigned char uniq[N];

	for (i = 0; i < N; i++)
		uniq[i] = 1;
	for (i = 0; i < N; i++) {
		if (uniq[wires[i]] == 0) {
			return 0;
		}
		uniq[wires[i]] = 0;
	}

	return 1;
}

/* Checks that the plugboard is connected in pairs, i.e. if for example
 * A is mapped to B, then B is mapped to A.
 */
static int is_plugboard_ok(struct plugboard *pb)
{
	int i;

	if (!are_wires_ok(pb->wires))
		return 0;

	for (i = 0; i < N; i++) {
		if (i != pb->wires[pb->wires[i]])
			return 0;
	}

	return 1;
}

static void check_plugboard(struct plugboard *pb)
{
	if (!is_plugboard_ok(pb)) {
		fprintf(stderr, "fatal: bad plugboard\n");
		exit(EXIT_FAILURE);
	}
}

static int plugboard_encode(struct plugboard *pb, int c)
{
	if (s_debug_on) {
		putchar((char) c + 'A');
		putchar((char) pb->wires[c] + 'A');
	}
	return pb->wires[c];
}

static void init_plugboard(struct plugboard *pb)
{
	int i;

	for (i = 0; i < N; i++) {
		pb->wires[i] = i;
	}

	check_plugboard(pb);
}

/* Connect the rotors wires. */
static void init(void)
{
	int i, r;
	struct rotor *pr;

	for (r = 0; r < NROTORS; r++) {
		pr = &s_rotors[r];
		for (i = 0; i < N; i++) {
			pr->wires[i] = pr->alphabet[i] - 'A';
			pr->rwires[pr->wires[i]] = i;
		}
		if (!are_wires_ok(pr->wires) ||
			!are_wires_ok(pr->rwires))
	       	{
			fprintf(stderr, "fatal: bad rotor %s\n",
				s_rotors[r].name);
			exit(EXIT_FAILURE);
		}
	}

	init_plugboard(&s_machine.pboard);
}

static int connect(struct rotor *pr, int base, int c)
{
	c = (c + (N - pr->ring) + base) % N;
	if (s_debug_on) {
		putchar((char) c + 'A');
		putchar((char) pr->wires[c] + 'A');
	}
	return (pr->ring + (N - base) + pr->wires[c]) % N;
}

static int rconnect(struct rotor *pr, int base, int c)
{
	c = (c + (N - pr->ring) + base) % N;
	if (s_debug_on) {
		putchar((char) c + 'A');
		putchar((char) pr->rwires[c] + 'A');
	}
	return (pr->ring + (N - base) + pr->rwires[c]) % N;
}

static int is_rotor_on_notch(struct rotor *pr, int base)
{
	int move;
	const char *c;

	move = 0;
	c = pr->stepwhen;
	while (*c != '\0') {
		move |= (base == *c - 'A');
		c++;
	}

	return move;
}

static void move_rotors(void)
{
	int i;
	int move[MACHINE_NROTORS];

	/* Each rotor will move if its pawn touches the right rotor notch.
	 * For the rightmost rotor it's like there is always a right rotor 
	 * notch.
	 */
	move[MACHINE_NROTORS - 1] = 1;
	for (i = MACHINE_NROTORS - 2; i >= 2; i--) {
		move[i] = is_rotor_on_notch(
			&s_rotors[s_machine.rotors[i + 1]],
			s_machine.base[i + 1]);
	}

	/* Double stepping:
	 * A rotor will move as well if a pawn touches its notch.
	 */
	for (i = 3; i < MACHINE_NROTORS; i++) {
		move[i] |= is_rotor_on_notch(
			&s_rotors[s_machine.rotors[i]],
			s_machine.base[i]);
	}

	for (i = 2; i < MACHINE_NROTORS; i++) {
		if (move[i]) {
			s_machine.base[i] = (s_machine.base[i] + 1) % N;
		}
	}
}

static int encode_char(int c)
{
	int i, lowcase;
	struct rotor *pr;

	lowcase = c >= 'a' && c <= 'z';
	if (lowcase || (c >= 'A' && c <= 'Z')) {
		move_rotors();

		if (lowcase)
			c -= 'a';
		else
			c -= 'A';

		if (s_debug_on) {
			putchar('(');
		}
		c = plugboard_encode(&s_machine.pboard, c);
		for (i = MACHINE_NROTORS - 1; i >= 0; i--) {
			pr = &s_rotors[s_machine.rotors[i]];
			c = connect(pr, s_machine.base[i], c);
		}
		for (i = 1; i < MACHINE_NROTORS; i++) {
			pr = &s_rotors[s_machine.rotors[i]];
			c = rconnect(pr, s_machine.base[i], c);
		}
		c = plugboard_encode(&s_machine.pboard, c);
		if (s_debug_on) {
			putchar(')');
		}
		if (lowcase)
			c += 'a';
		else
			c += 'A';
	}

	return c;
}

static void encode(char *s)
{
	int c;

	while (*s != '\0') {
		c = encode_char(*s++);
		putchar((char) c);
	}
}

static void run_in(void)
{
	char *tok;

	printf(">>>>");
	tok = strtok(NULL, TOKSEP);
	while (tok != NULL) {
		printf(" ");
		encode(tok);
		tok = strtok(NULL, TOKSEP);
	}
	putchar('\n');
}

static void reset_bases(void)
{
	int i;

	for (i = 0; i < MACHINE_NROTORS; i++) {
		s_machine.base[i] = 0;
	}
}

/* Print current machine settings. */
static void run_config(void)
{
	int i;

	for (i = 0; i < MACHINE_NROTORS; i++) {
		printf("%s ", s_rotors[s_machine.rotors[i]].name);
	}
	for (i = 1; i < MACHINE_NROTORS; i++) {
		printf("%02d ", s_rotors[s_machine.rotors[i]].ring + 1);
	}
	for (i = 1; i < MACHINE_NROTORS; i++) {
		putchar((char) (s_machine.base[i] + 'A'));
	}
	for (i = 0; i < N; i++) {
		if (s_machine.pboard.wires[i] != i
			&& i < s_machine.pboard.wires[i])
	       	{
			printf(" %c%c", (char) (i + 'A'),
				(char) (s_machine.pboard.wires[i] + 'A'));
		}
	}
	putchar('\n');
}

/* Gets the rotor type (ROTOR_1, ROTOR_B, etc) from the name I, B, etc. */
static int get_rotor_id_by_name(const char *name)
{
	int i;

	for (i = 0; i < NROTORS; i++) {
		if (streq(s_rotors[i].name, name))
			return i;
	}
	return -1;
}

/* Returns 1 if n in [a, b]. */
static int in_range(int n, int a, int b)
{
	return n >= a && n <= b;
}

/* Set contains ROTOR_1, etc, and has MACHINE_NROTORS elements. */
static int is_rotor_setting_ok(int set[])
{
	int i, j;

	if (!in_range(set[0], ROTOR_B, ROTOR_C))
		return 0;

	if (!in_range(set[1], ROTOR_BETA, ROTOR_GAMMA))
		return 0;

	for (i = 2; i < MACHINE_NROTORS; i++) {
		if (!in_range(set[i], ROTOR_1, ROTOR_8))
			return 0;
	}

	/* This checks that we don't use the same rotor type twice.
	 * It cannot be disabled as the code does not allow for
	 * rotor duplicates right now...
	 */
	for (i = 0; i < MACHINE_NROTORS; i++) {
		for (j = 0; j < MACHINE_NROTORS; j++) {
			if (i == j)
				continue;
			if (set[i] == set[j])
				return 0;
		}
	}

	return 1;
}

static int run_rotors(void)
{
	int i;
	char *tok;
	int set[MACHINE_NROTORS];

	for (i = 0; i < MACHINE_NROTORS; i++) {
		set[i] = -1;
		tok = strtok(NULL, TOKSEP);
		if (tok != NULL) {
			set[i] = get_rotor_id_by_name(tok);
		}
	}

	if (!is_rotor_setting_ok(set)) {
		printf("Bad rotor setting\n");
		return 0;
	}

	for (i = 0; i < MACHINE_NROTORS; i++) {
		s_machine.rotors[i] = set[i];
		s_rotors[s_machine.rotors[i]].ring = 0;
	}

	reset_bases();
	return 1;
}

static int run_rings(void)
{
	int i, rotori;
	char *tok;
	int set[MACHINE_NROTORS];

	memset(set, 0, sizeof(set));
	for (i = 1; i < MACHINE_NROTORS; i++) {
		tok = strtok(NULL, TOKSEP);
		if (tok != NULL)
			set[i] = atoi(tok);
		if (tok == NULL || set[i] <= 0 || set[i] > 26) {
			printf("Bad ring setting\n");
			return 0;
		}
	}

	for (i = 1; i < MACHINE_NROTORS; i++) {
		rotori = s_machine.rotors[i];
		s_rotors[rotori].ring = set[i] - 1;
	}

	reset_bases();
	return 1;
}

static int run_bases(void)
{
	int i, a;
	char *tok;
	int base[MACHINE_NROTORS];

	tok = strtok(NULL, TOKSEP);
	if (tok == NULL || strlen(tok) != MACHINE_NROTORS - 1)
		goto bad;

	for (i = 0; i < MACHINE_NROTORS - 1; i++) {
		a = tonum(tok[i]);
		if (a >= 0 && a < N) {
			base[i + 1] = a;
		} else {
			goto bad;
		}
	}

	for (i = 1; i < MACHINE_NROTORS; i++) {
		s_machine.base[i] = base[i];
	}

	return 1;

bad:
	printf("Bad formatted bases\n");
	return 0;
}

/* Unplug the code 'id; be sure that the pair is unplugged. */
static void plugboard_unplug(struct plugboard *pb, int id)
{
	int i;

	i = pb->wires[id];
	if (i != id) {
		pb->wires[i] = i;
		pb->wires[id] = id;
	}
}

static int run_plug(void)
{
	int i, a, b;
	char *tok;
	unsigned char plugs[N];

	for (i = 0; i < N; i++) {
		plugs[i] = N;
	}

	tok = strtok(NULL, TOKSEP);
	while (tok != NULL) {
		if (strlen(tok) != 2)
			goto bad;
		a = tonum(tok[0]);
		b = tonum(tok[1]);
		if (a >= 0 && a < N && b >= 0 && b < N) {
			plugs[a] = b;
			tok = strtok(NULL, TOKSEP);
		} else {
			goto bad;
		}
	}

	for (i = 0; i < N; i++) {
		if (plugs[i] != N) {
			plugboard_unplug(&s_machine.pboard, i);
			plugboard_unplug(&s_machine.pboard, plugs[i]);
			s_machine.pboard.wires[i] = plugs[i];
			s_machine.pboard.wires[plugs[i]] = i;
		}
	}

	check_plugboard(&s_machine.pboard);
	return 1;

bad:
	printf("Bad plugboard setting\n");
	return 0;
}

static int run_unplug(void)
{
	int i, a;
	char *tok;
	unsigned char unplugs[N];

	for (i = 0; i < N; i++) {
		unplugs[i] = 0;
	}

	memset(unplugs, 0, sizeof(unplugs));
	tok = strtok(NULL, TOKSEP);
	if (tok != NULL && streq(tok, "all")) {
		for (i = 0; i < N; i++) {
			unplugs[i] = 1;
		}
	} else for (i = 0; tok[i] != '\0'; i++) {
		a = tonum(tok[i]);
		if (a >= 0 && a < N)
			unplugs[a] = 1;
		else
			goto bad;
	}

	for (i = 0; i < N; i++) {
		if (unplugs[i]) {
			plugboard_unplug(&s_machine.pboard, i);
		}
	}

	check_plugboard(&s_machine.pboard);
	return 1;

bad:
	printf("Bad plugboard setting\n");
	return 0;
}

static void run_help(void)
{
	printf("> rotors (ex. rotors b Beta I II VIII)\n");
      	printf("        Set the machine rotors. Available rotors:\n");
	printf("        b, c, Beta, Gamma, I, II, ... VIII.\n");
	printf("> rings (ex. rings 1 1 7 26)\n");
      	printf("        Set the ring setting for the installed rotors.\n");
	printf("> bases (ex. bases AAXR)\n");
      	printf("        Set the rotor positions for the installed rotors.\n");
	printf("> plug (ex. plug AJ PS RT)\n");
      	printf("        Plug the given signals on the plugboard.\n");
	printf("> unplug (ex. unplug all) (ex. unplug AFRT)\n");
      	printf("        Unplug all plugboard signals or the given signals.\n");
	printf("> in (ex. in HELLO)\n");
      	printf("        Enter the characters for ciphering.\n");
	printf("> config        Print the current machine settings.\n");
	printf("> debug         Switch debug mode.\n");
	printf("> help          Show this help.\n");
	printf("> quit          Exit the program.\n");
}

static void run_debug(void)
{
	s_debug_on = !s_debug_on;
	if (s_debug_on) {
		printf("Debug On\n");
	} else {
		printf("Debug Off\n");
	}
}

static int set_key(const char *key)
{
	int r;
	char line[256];

	snprintf(line, sizeof(line), "key %s", key);
	strtok(line, TOKSEP); 
	r = run_rotors();
	if (r)
		r = run_rings();
	if (r)
		r = run_bases();
	if (r)
		r = run_plug();
	return r;
}

static int run_line(char *s)
{
	char *tok;
	int r, prconfig;

	r = 1;
	prconfig = 0;
	tok = strtok(s, TOKSEP);
	if (tok == NULL) {
		/* nothing */
	} else if(streq(tok, "quit")) {
		r = 0;
	} else if (streq(tok, "in")) {
		run_in();
	} else if (streq(tok, "debug")) {
		run_debug();
	} else if (streq(tok, "rotors")) {
		prconfig = run_rotors();
	} else if (streq(tok, "rings")) {
		prconfig = run_rings();
	} else if (streq(tok, "bases")) {
		prconfig = run_bases();
	} else if (streq(tok, "plug")) {
		prconfig = run_plug();
	} else if (streq(tok, "unplug")) {
		prconfig = run_unplug();
	} else if (streq(tok, "config")) {
		run_config();
	} else if (streq(tok, "help")) {
		run_help();
	} else {
		printf("unknown command\n");
	}
	if (prconfig) {
		run_config();
	}
	return r;
}

static const char *s_version[] =
{
PACKAGE_STRING,
"",
"Copyright (C) 2016-2017 Jorge Giner Cordero.",
"This is free software: you are free to change and redistribute it.",
"There is NO WARRANTY, to the extent permitted by law."
};

static void print_version(FILE *f)
{
	int i;

	for (i = 0; i < NELEMS(s_version); i++)
		fprintf(f, "%s\n", s_version[i]);
}

static void loop(void)
{
	int r, c;
	size_t len;
	char line[75];

	print_version(stdout);
	printf("\nType 'help' for the list of available commands.\n\n");
	run_config();
	printf("> ");
	while (fgets(line, sizeof(line), stdin) != NULL) {
		len = strlen(line);
		if (len > 0 && line[len - 1] != '\n') {
			printf("Line too long. Nothing done.\n");
			do {
				c = getchar();
			} while (c != EOF && c != '\n');
		} else {
			r = run_line(line);
			if (!r)
				return;
		}
		printf("> ");
	}
}

static void encode_files(void)
{
	int c;
	FILE *fps, *fpd;

	if ((fps = fopen(s_srcfname, "rb")) == NULL) {
		fprintf(stderr, PACKAGE ": cannot open %s\n", s_srcfname);
		return;
	}

	if ((fpd = fopen(s_dstfname, "wb")) == NULL) {
		fclose(fps);
		fprintf(stderr, PACKAGE ": cannot open %s\n", s_dstfname);
		return;
	}

	while ((c = getc(fps)) != EOF) {
		c = encode_char(c);
		putc(c, fpd);
	}

	fclose(fpd);
	fclose(fps);
}

static void filter(void)
{
	int c;

	while ((c = getc(stdin)) != EOF) {
		c = encode_char(c);
		putchar((char) c);
	}
}

static void print_help(const char *argv0)
{
	static const char *help =
"Usage: %s [OPTION]... [ SOURCE DEST ]\n"
"\n"
"If SOURCE and DEST are specified, encode the SOURCE file into the DEST\n"
"file. If not, the program will run in interactive mode, unless the option\n"
"--filter is given, in which case stdin will be encoded to stdout.\n"
"\n"
"Options:\n"
"  -f, --filter        Encode stdin to stdout.\n"      	
"  -h, --help          Display this help and exit.\n"
"  -k, --key=KEY       Set the initial machine configuration. For example:\n"
"                      -k \"c Gamma V IV I 1 26 2 3 RTJZ BT RJ\".\n"
"  -v, --version       Output version information and exit.\n"
"\n"
"Report bugs to: <" PACKAGE_BUGREPORT ">.\n"
"Home page: <" PACKAGE_URL ">.\n"
;

	printf(help, argv0);
}

static void handle_options(int argc, char *argv[])
{
	int c;
	struct ngetopt ngo;

	static struct ngetopt_opt ops[] = {
		{ "version", 0, 'v' },
		{ "help", 0, 'h' },
		{ "key", 1, 'k' },
		{ "filter", 0, 'f' },
		{ NULL, 0, 0 },
	};

	ngetopt_init(&ngo, argc, argv, ops);
	do {
		c = ngetopt_next(&ngo);
		switch (c) {
		case 'v':
			print_version(stdout);
			exit(EXIT_SUCCESS);
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'k':
			if (!set_key(ngo.optarg)) {
				exit(EXIT_FAILURE);
			}
			break;
		case 'f':
			s_mode = MODE_FILTER;
			break;
		case '?':
			fprintf(stderr, PACKAGE ": unrecognized option %s\n",
				ngo.optarg);
			exit(EXIT_FAILURE);
		case ':':
			fprintf(stderr, PACKAGE ": %s needs an argument\n",
				ngo.optarg);
			exit(EXIT_FAILURE);
		case ';':
			fprintf(stderr, PACKAGE
				": %s does not allow for arguments\n",
				ngo.optarg);
			exit(EXIT_FAILURE);
		}
	} while (c != -1);

	if (argc > ngo.optind) {
		s_srcfname = argv[ngo.optind];
	}

	if (argc > ngo.optind + 1) {
		s_dstfname = argv[ngo.optind + 1];
	}

	if (s_srcfname != NULL && s_dstfname == NULL) {
		fprintf(stderr, PACKAGE": unspecified DEST file\n");
		exit(EXIT_FAILURE);
	}

	if (s_dstfname != NULL) {
		if (s_mode == MODE_FILTER) {
			fprintf(stderr, PACKAGE
				": warning: --filter option ignored\n");
		}
		s_mode = MODE_FILE;
	}
}

int main(int argc, char *argv[])
{
	init();
	handle_options(argc, argv);
	switch (s_mode) {
	case MODE_LOOP: loop(); break;
	case MODE_FILTER: filter(); break;
	case MODE_FILE: encode_files(); break;
	}
	return EXIT_SUCCESS;
}
