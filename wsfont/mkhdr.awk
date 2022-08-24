#!/usr/bin/awk -f

BEGIN {
	N = 0
}

{
	FNAME = FILENAME
	gsub(/^.+\//, "", FNAME)
	sub(/\.h$/, "", FNAME)
	if (FNAME ~ /vt220koi8x10/)
		nextfile	# we get a macro--skip this

	PAT = "(static[[:blank:]]+)?struct[[:blank:]]+wsdisplay_font[[:blank:]]+[^[:blank:]]+[[:blank:]]*=[[:blank:]]*{"
	if ($0 ~ PAT) {
		printf("#include <dev/wsfont/%s.h>\n", FNAME)
		PFX = "(static[[:blank:]]+)?struct[[:blank:]]+wsdisplay_font[[:blank:]]+"
		SUF = "[[:blank:]]*=[[:blank:]]*{"
		FONT = $0
		sub(PFX, "", FONT)
		sub(SUF, "", FONT)
		if (length FONT > 0)
			FONTS[N++] = FONT
		nextfile
	}
}
END {
	printf("\nstatic struct wsfont {\n")
	printf("\tchar* fname;\n")
	printf("\tstruct wsdisplay_font* wsdfont;\n")
	printf("} wsfonts[] = {\n")
	for (i = 0; i < N; i++)
		printf("\t{ \"%s\", \t&%s },\n", FONTS[i], FONTS[i])
	printf("\t{ NULL, NULL }\n};\n");
}
