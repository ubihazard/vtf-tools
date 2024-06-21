#include "vtfshared.hxx"

int main (int argc, char** argv)
{
	using namespace VTFLib;

	// Expecting a single input
	const auto* const app = argv[0];
	++argv, --argc;

	auto force = FALSE;
	auto regen = FALSE;

	if (argc == 2) {
		     if (strieq(argv[1], "force")) {force = TRUE; --argc;}
		else if (strieq(argv[1], "regen")) {force = TRUE; regen = TRUE; --argc;}
	}

	if (argc != 1) {
		printf("Error: only one texture must be specified (got %d)\n", argc);
usage:
		printf("Usage: %s input.vtf [force | regen]\n", app);
		return (argc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// Load the texture
	auto vtf = CVTFFile{};
	if (!vtf.Load(argv[0])) {
		printf("Error: couldn't load \"%s\"\n", argv[0]);
		goto usage;
	}

	// Find out the thumbnail size
	auto old_thumb_size = 0;
	if (char(vtf.GetThumbnailFormat()) != IMAGE_FORMAT_NONE) {
		old_thumb_size = vtf.GetThumbnailDataSize();
		if (not force) {
			printf("Error: \"%s\" already got thumbnail\n", argv[0]);
			return EXIT_FAILURE;
		}
	} else if (regen) {
		printf("Error: \"%s\" got no thumbnail\n", argv[0]);
		return EXIT_FAILURE;
	}

	auto width = vtf.GetWidth();
	auto height = vtf.GetHeight();
	auto base = max(width, height);
	while (base != 16) {
		base >>= 1;
		width >>= 1;
		height >>= 1;
	}
	width = max(width, 1);
	height = max(height, 1);

	// Calculate the size of the thumbnail
	auto vtf_size = vtf_get_real_size(vtf);
	auto const thumb_size = vtf.ComputeImageSize(width
	, height, 1, IMAGE_FORMAT_DXT1);
	auto const img_size = vtf.GetImageDataSize();
	auto const vtf_size_new = VTF_HDR_SIZE + thumb_size + img_size;

	// Make a copy
	auto const vtf_mem = malloc(max(vtf_size, vtf_size_new));
	if (vtf_mem == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	vtf.Save(vtf_mem, vtf_size, vtf_size);

	// Drop the resource block
	vtf_force_72(vtf, vtf_mem, true);
	memmove((char*)vtf_mem + VTF_HDR_SIZE + thumb_size
	, (char*)vtf_mem + VTF_HDR_SIZE + old_thumb_size
	, img_size);
	*(int*)((char*)vtf_mem + VTF_HDR_OFF_THUMB_FMT) = IMAGE_FORMAT_DXT1;
	*((char*)vtf_mem + VTF_HDR_OFF_THUMB_W) = width;
	*((char*)vtf_mem + VTF_HDR_OFF_THUMB_H) = height;
	if (!vtf.Load(vtf_mem, vtf_size_new, FALSE)) {
		printf("Error: couldn't load modified VTF\n");
		return EXIT_FAILURE;
	}

	// Generate the thumbnail
	if (!vtf.GenerateThumbnail()) {
		printf("Error: couldn't generate thumbnail for \"%s\"\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (!vtf.Save(argv[0])) {
		printf("Error: couldn't save \"%s\"\n", argv[0]);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
