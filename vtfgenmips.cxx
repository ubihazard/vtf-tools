#include "vtfshared.hxx"

int main (int argc, char** argv)
{
	using namespace VTFLib;

	auto filter = MIPMAP_FILTER_BOX;
	auto sharpen = SHARPEN_FILTER_NONE;

	// Expecting a single input
	const auto* const app = argv[0];
	++argv, --argc;

	if (argc < 1) {
		printf("Error: a texture must be specified\n");
usage:
		printf("Usage: %s input.vtf [<resize filter>] [<sharpen filter>]\n", app);
		if (argc == 0) goto known_resize_filters;
		return EXIT_FAILURE;
	}

	if (argc >= 2) {
		const auto* const arg_filter = argv[1];
		     if (strieq(arg_filter, "point"))     {filter = MIPMAP_FILTER_POINT;}
		else if (strieq(arg_filter, "box"))       {filter = MIPMAP_FILTER_BOX;}
		else if (strieq(arg_filter, "triangle"))  {filter = MIPMAP_FILTER_TRIANGLE;}
		else if (strieq(arg_filter, "quadratic")) {filter = MIPMAP_FILTER_QUADRATIC;}
		else if (strieq(arg_filter, "cubic"))     {filter = MIPMAP_FILTER_CUBIC;}
		else if (strieq(arg_filter, "catrom"))    {filter = MIPMAP_FILTER_CATROM;}
		else if (strieq(arg_filter, "mitchell"))  {filter = MIPMAP_FILTER_MITCHELL;}
		else if (strieq(arg_filter, "gaussian"))  {filter = MIPMAP_FILTER_GAUSSIAN;}
		else if (strieq(arg_filter, "sinc"))      {filter = MIPMAP_FILTER_SINC;}
		else if (strieq(arg_filter, "bessel"))    {filter = MIPMAP_FILTER_BESSEL;}
		else if (strieq(arg_filter, "hanning"))   {filter = MIPMAP_FILTER_HANNING;}
		else if (strieq(arg_filter, "hamming"))   {filter = MIPMAP_FILTER_HAMMING;}
		else if (strieq(arg_filter, "blackman"))  {filter = MIPMAP_FILTER_BLACKMAN;}
		else if (strieq(arg_filter, "kaiser"))    {filter = MIPMAP_FILTER_KAISER;}
		else {
			printf("Error: unknown resize filter: %s\n", arg_filter);
known_resize_filters:
			printf("Known resize filters:\n");
			printf("\tbox (default)\n");
			printf("\ttriangle\n");
			printf("\tquadratic\n");
			printf("\tcubic\n");
			printf("\tcatrom\n");
			printf("\tmitchell\n");
			printf("\tgaussian\n");
			printf("\tsinc\n");
			printf("\tbessel\n");
			printf("\thanning\n");
			printf("\thamming\n");
			printf("\tblackman\n");
			printf("\tkaiser\n");
			if (argc == 0) goto known_sharpen_filters;
			goto usage;
		}
	}

	if (argc == 3) {
		const auto* const arg_filter = argv[2];
		     if (strieq(arg_filter, "none"))      {sharpen = SHARPEN_FILTER_NONE;}
		else if (strieq(arg_filter, "negative"))  {sharpen = SHARPEN_FILTER_NEGATIVE;}
		else if (strieq(arg_filter, "lighter"))   {sharpen = SHARPEN_FILTER_LIGHTER;}
		else if (strieq(arg_filter, "darker"))    {sharpen = SHARPEN_FILTER_DARKER;}
		else if (strieq(arg_filter, "contrastmore")) {sharpen = SHARPEN_FILTER_CONTRASTMORE;}
		else if (strieq(arg_filter, "contrastless")) {sharpen = SHARPEN_FILTER_CONTRASTLESS;}
		else if (strieq(arg_filter, "smoothen"))  {sharpen = SHARPEN_FILTER_SMOOTHEN;}
		else if (strieq(arg_filter, "sharpensoft")) {sharpen = SHARPEN_FILTER_SHARPENSOFT;}
		else if (strieq(arg_filter, "sharpenmedium")) {sharpen = SHARPEN_FILTER_SHARPENMEDIUM;}
		else if (strieq(arg_filter, "sharpenstrong")) {sharpen = SHARPEN_FILTER_SHARPENSTRONG;}
		else if (strieq(arg_filter, "findedges")) {sharpen = SHARPEN_FILTER_FINDEDGES;}
		else if (strieq(arg_filter, "contour"))   {sharpen = SHARPEN_FILTER_CONTOUR;}
		else if (strieq(arg_filter, "edgedetect")) {sharpen = SHARPEN_FILTER_EDGEDETECT;}
		else if (strieq(arg_filter, "edgedetectsoft")) {sharpen = SHARPEN_FILTER_EDGEDETECTSOFT;}
		else if (strieq(arg_filter, "emboss"))    {sharpen = SHARPEN_FILTER_EMBOSS;}
		else if (strieq(arg_filter, "meanremoval")) {sharpen = SHARPEN_FILTER_MEANREMOVAL;}
		else if (strieq(arg_filter, "unsharp"))   {sharpen = SHARPEN_FILTER_UNSHARP;}
		else if (strieq(arg_filter, "xsharpen"))  {sharpen = SHARPEN_FILTER_XSHARPEN;}
		else if (strieq(arg_filter, "warpsharp")) {sharpen = SHARPEN_FILTER_WARPSHARP;}
		else {
			printf("Error: unknown sharpen filter: %s\n", arg_filter);
known_sharpen_filters:
			printf("Known sharpen filters:\n");
			printf("\tnone (default)\n");
			printf("\tnegative\n");
			printf("\tlighter\n");
			printf("\tdarker\n");
			printf("\tcontrastmore\n");
			printf("\tcontrastless\n");
			printf("\tsmoothen\n");
			printf("\tsharpensoft\n");
			printf("\tsharpenmedium\n");
			printf("\tsharpenstrong\n");
			printf("\tfindedges\n");
			printf("\tcontour\n");
			printf("\tedgedetect\n");
			printf("\tedgedetectsoft\n");
			printf("\temboss\n");
			printf("\tmeanremoval\n");
			printf("\tunsharp\n");
			printf("\txsharpen\n");
			printf("\twarpsharp\n");
			if (argc == 0) return EXIT_SUCCESS;
			goto usage;
		}
	}

	if (argc > 3) {
		goto usage;
	}

	// Load the texture
	auto vtf = CVTFFile{};
	if (!vtf.Load(argv[0])) {
		printf("Error: couldn't load \"%s\"\n", argv[0]);
		goto usage;
	}

	// Find out the number of mipmaps to generate
	auto const width = vtf.GetWidth();
	auto const height = vtf.GetHeight();
	auto const depth = vtf.GetDepth();
	auto mips = vtf.GetMipmapCount();
	auto const frames = vtf.GetFrameCount();
	auto const format = vtf.GetFormat();

	if (mips != 1) {
		printf("Error: \"%s\" already got mipmaps\n", argv[0]);
		return EXIT_FAILURE;
	}

	auto base = min(width, height);
	while (base != 1) {
		base >>= 1;
		++mips;
	}

	// Calculate the new VTF size
	vlUInt img_size_add = 0;
	auto m = mips;
	while (m-- != 1) {
		auto const mip_size = vtf.ComputeImageSize(width >> m
		, height >> m, depth, format);
		img_size_add += mip_size;
	}
	img_size_add *= frames;

	// Calculate the size of the current mipmap
	auto vtf_size = vtf_get_real_size(vtf);
	auto const thumb_size = vtf.GetThumbnailDataSize();
	auto const img_size = vtf.GetImageDataSize();
	auto const vtf_size_new = VTF_HDR_SIZE + thumb_size + img_size + img_size_add;
	auto const off = VTF_HDR_SIZE + thumb_size;

	// Make a copy
	auto const vtf_mem = malloc(max(vtf_size, vtf_size_new));
	if (vtf_mem == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	vtf.Save(vtf_mem, vtf_size, vtf_size);

	// Drop the resource block
	vtf_force_72(vtf, vtf_mem, true);
	memmove((char*)vtf_mem + off + img_size_add
	, (char*)vtf_mem + off
	, img_size);
	*(char*)((char*)vtf_mem + VTF_HDR_OFF_MIPS) = mips;
	vtf.Load(vtf_mem, vtf_size_new, FALSE);

	// Generate the mipmaps
	if (!vtf.GenerateMipmaps(filter, sharpen)) {
		printf("Error: couldn't generate mipmaps for \"%s\"\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Restore the original high resolution mipmap
	auto const vtf_new = malloc(vtf_size_new);
	if (vtf_new == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	vtf.Save(vtf_new, vtf_size_new, vtf_size);

	auto const mip_size = vtf.ComputeImageSize(width, height, depth, format) * frames;
	memcpy((char*)vtf_new + vtf_size_new - mip_size
	, (char*)vtf_mem + vtf_size_new - mip_size
	, mip_size);

	// Save new VTF
	auto const f = fopen(argv[0], "wb");
	if (f == NULL) {
		printf("Error: couldn't write \"%s\"\n", argv[0]);
		goto fail;
	}
	fwrite(vtf_new, 1, vtf_size_new, f);
	fclose(f);

	// Cleanup
	free(vtf_new);

	return EXIT_SUCCESS;

fail:
	free(vtf_new);

	return EXIT_FAILURE;
}
