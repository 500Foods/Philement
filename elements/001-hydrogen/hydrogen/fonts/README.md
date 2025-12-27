# Fonts

The Iosevka font tool has been used to generate a suite of custom fonts. The main highlights include the following.

- "Open" 4's - meaning the top of the for is separated
- Alternate 0 - a slash in the zero to distinguish it visually from an O
- Tabular numbers - fixed-width numbers that align even in the proportional font
- Many other glyph tweaks

The [private-build-plans.toml](/elements/001-hydrogen/hydrogen/fonts/private-build-plans.toml) file contains the configuration choices for these fonts.

- Acuranzo Sans - Typical proportional font
- Acuranzo Mono - Monospaced font for terminal and code editing purposes
- Acuranzo Sans Fancy - Same as above but with "fancy" ligatures (replacing common symbols with fancier symbols)
- Acuranzo Mono Fancy - Same as above but with "fancy" ligatures (replacing common symbols with fancier symbols)
- Acuranzo Sans Mail - A stripped down version suitable for embedding in emails as base64
- Acuranzo Mono Mail - A stripped down version suitable for embedding in emails as bas64

The Iosevka tool is, shall we say, *comprehensive* in what it generates - potentially hundreds of woff2 files, not to mention any other font format desired.
For the Hydrogen project, we've selected a few that are included here that we'll use directly, just know that more are available if needed using the build
file indicated above.

NOTE: Depending on the build options chosen, it can take more than hour or more to build a comprehensive set of fonts.

NOTE: The Mail variants were created to have a very small file (less than 15 KB each) that can be directly embedded as Base64 in HTML emails to avoid the
dreaded "download remote content?" message in mail clients like Thunderbird and iOS. GMail doesn't even support fonts, so no help there. The normal non-Mail
fonts are typically arund 1.6 MB. Far too large to be arbitrarily embedding in emails. The Mail versions are *very* stripped down character sets, not much
more than typical ASCII characters, but can be augmented easily enough by adding individual codepoints to the cnfiguration file as needed.

## References

[Iosevka](https://github.com/be5invis/Iosevka?tab=readme-ov-file) - GitHub repository for the project
[Iosevka Customizer](https://typeof.net/Iosevka/customizer) - Used to generate configuration file
