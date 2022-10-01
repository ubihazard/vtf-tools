#include "vtfshared.hxx"

int main (int argc, char** argv)
{
	using namespace VTFLib;

	// Expecting a single input
	const auto* const app = argv[0];
	++argv, --argc;

	if (argc != 1) {
		printf("Error: only one texture must be specified (got %d)\n", argc);
usage:
		printf("Usage: %s input.vtf\n", app);
		return (argc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// Load the texture
	auto vtf = CVTFFile{};
	if (!vtf.Load(argv[0])) {
		printf("Error: couldn't load \"%s\"\n", argv[0]);
		goto usage;
	}

	// Get the texture parameters
	auto const width = vtf.GetWidth();
	auto const height = vtf.GetHeight();
	auto const depth = vtf.GetDepth();
	auto const mips = vtf.GetMipmapCount();
	auto const frames = vtf.GetFrameCount();
	auto const format = vtf.GetFormat();
	auto const flags = vtf.GetFlags();

	if (mips == 1) {
		printf("Error: \"%s\": can't remove the last mipmap\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Calculate the size of the largest mipmap
	auto vtf_size = vtf_get_real_size(vtf);
	auto const thumb_size = vtf.GetThumbnailDataSize();
	auto const img_size = vtf.GetImageDataSize();
	auto const mip_size = vtf.ComputeImageSize(width, height, depth, format) * frames;
	auto const vtf_size_new = VTF_HDR_SIZE + thumb_size + img_size - mip_size;

	// Copy the VTF
	auto const vtf_new = malloc(max(vtf_size, vtf_size_new));
	if (vtf_new == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	vtf.Save(vtf_new, vtf_size, vtf_size);

	// Preserve all mipmaps except the largest
	vtf_force_72(vtf, vtf_new, true);
	if (mips - 1 == 1) {
		*(unsigned int*)((char*)vtf_new + VTF_HDR_OFF_FLAGS) = flags
		| TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD;
	}
	*(short*)((char*)vtf_new + VTF_HDR_OFF_WIDTH) = width >> 1;
	*(short*)((char*)vtf_new + VTF_HDR_OFF_HEIGHT) = height >> 1;
	*(char*)((char*)vtf_new + VTF_HDR_OFF_MIPS) = mips - 1;

	// Write the file
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
