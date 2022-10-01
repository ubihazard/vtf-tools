<h1 align="center">VTF Lossless Tools</h1>

<!--
# VTF Lossless Tools
-->

Command-line tools for manipulating [VTF](https://developer.valvesoftware.com/wiki/Valve_Texture_Format) (Valve Texture Format) textures losslessly. These utilities allow to quickly perform and automate various tedious tasks that typically require re-creating (and re-encoding) textures from scratch, potentially loosing image quality.

Download the [latest release](https://github.com/ubihazard/vtf-tools/releases).

Some of the tools mentioned below can be used to alter DXT-compressed texture image data directly. The catch is that they allow to modify DXT1 and DXT5 encoded VTF textures losslessly: that is, without re-encoding them again in DXT format and further reducing image quality.

  * `vtfsplit`: separate animated VTF texture into individual frame files.
  * `vtfmerge`: merge the individual VTF frames back into animated VTF texture.
  * `vtfpatch`[^1]: copy a rectangular portion of DXT data from source VTF into the destination VTF. Both color channel and alpha channel can be copied together. Only the largest mipmap is affected.
  * `vtfcopyalpha`[^1]: copy the entire alpha channel from source VTF into the destination VTF (all mipmaps).
  * `vtfzapalpha`[^1]: discard the alpha channel. Can be used to remove unnecessary (empty) or no longer needed alpha channel and reduce VTF size.
  * `vtfzapmain`: discard the largest mipmap (halves the texture dimensions).
  * `vtfzapmips`: discard all but the largest mipmap (no LOD).
  * `vtfzapthumb`: discard the VTF thumbnail mini-texture.
  * `vtfzapreflectivity`: reset the VTF reflectivity values.
  * `vtfgenmips`: re-create the entire set of mipmaps from the current mipmap (which is largest).
  * `vtfgenthumb`: re-generate the VTF thumbnail.
  * `vtfflags`: alter (add / remove / toggle) VTF flags.
  * `vtf72`: convert the VTF to version 7.2 format.

[^1]: tools that work only with DXT-compressed VTF textures.

Invoke each tool without command-line arguments for usage instructions.

*Note that using any of these tools will convert the VTF to version 7.2 format.* This is required to remove the potential CRC checksum attached to VTF after VTF image data has been changed. So if you use any of 7.3+ VTF features, like resources for UV LOD settings or CRC, for example, you would have to re-save the modified VTF texture with these features re-added using the appropriate tool.

## How to Use

Download [VTFEdit](https://nemstools.github.io/files/vtfedit133.zip) which contains the required DLLs. Extract the downloaded `vtftools` package into the appropriate VTFEdit architecture subfolder and invoke all utilities from there.

## How to Build

Download [VTFLib](https://nemstools.github.io/files/vtflib132.zip) which contains the required libraries. Extract the `lib` folder into the `vtftools` source folder and run the correct `.bat` file to build VTF tools of appropriate architecture. Do not extract the `VTFLib` folder, – VTF tools use the slightly modified version of `VTFLib` source code to assist its building process.

## Using `vtfpatch`

The best workflow is to use Adobe Photoshop to apply the [DXT compression fix](https://developer.valvesoftware.com/wiki/Fixing_DXT_Green_Tint_Compression), manually edit the texture, and re-export it in VTF format with all mipmaps. Then, use `vtfpatch` over the entire texture dimensions (e.g. 0,0,1023,1023) to restore the original largest mipmap from the unmodified VTF. Finally, use `vtfpatch` again to copy over only the patched region you originally were intending to “patch” in the largest mipmap. This takes quite a bit of time, but gives the best result.

The much faster alternative is to use `vtfpatch` right away, and then simply `vtfzapmips` followed by `vtfgenmips` to re-generate mipmaps with patch included. However, the generated mipmaps wouldn’t carry the DXT color compression fix and would have their colors slightly off.

## ⭐ Support

Making quality software is hard and time-consuming. If you find [VTF tools](https://github.com/ubihazard/vtf-tools) useful, you can [buy me a ☕](https://www.buymeacoffee.com/ubihazard "Donate")!
