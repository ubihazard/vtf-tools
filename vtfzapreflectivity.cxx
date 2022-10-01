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

	// Calculate the new VTF size
	auto vtf_size = vtf_get_real_size(vtf);
	auto const thumb_size = vtf.GetThumbnailDataSize();
	auto const img_size = vtf.GetImageDataSize();
	auto const vtf_size_new = VTF_HDR_SIZE + thumb_size + img_size;

	// Copy the VTF
	auto const vtf_new = malloc(max(vtf_size, vtf_size_new));
	if (vtf_new == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	vtf.Save(vtf_new, vtf_size, vtf_size);

	// Get rid of the reflectivity
	vtf_force_72(vtf, vtf_new, true);
	*((float*)((char*)vtf_new + VTF_HDR_OFF_REFL) + 0) = 1;
	*((float*)((char*)vtf_new + VTF_HDR_OFF_REFL) + 1) = 1;
	*((float*)((char*)vtf_new + VTF_HDR_OFF_REFL) + 2) = 1;

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
