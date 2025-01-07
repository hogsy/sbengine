# Starbreeze Engine Sources

Here are several copies of the source-code for Starbreeze’s in-house engine that were obtained and sorted from an earlier leak from Sony's private developer forum. Starbreeze has long since ditched this engine, but mind that it is still their intellectual property and could get taken down at any time, and has not been made officially open-source in any capacity.

Much of the platform-specific code, besides that for the PlayStation 3, has been stripped out by the developer that originally uploaded these prior to them being leaked.

This is available here for the sake of preservation, research and nothing more. Assets are not included (not that they were even available).

You can find the original packages containing the source code and more [here](https://archive.org/details/sbengine).

**Mind that the source files seem to use ISO-8859-1 encoding, so be sure to configure that in your editor if you don't want to mess things up!**

## Further Observations

### Name?

So I've seen some mistakenly think that Ogier is the name of the engine - it's not, that's the name of the editor, and I've really never understood how that misunderstanding came about, but it infuriates me to no end.

At least [as far back as 2004](https://web.archive.org/web/20041214184257/http://www.starbreeze.com/engine.jsp), it's been referred to as the Starbreeze Engine and was still referred to as such when their final game to use the technology, Syndicate, was released in 2012.

A possibly earlier name, 'Extended Reality Engine', is mentioned, which is very likely why an XR prefix is used in various places.

### Tooling

The source code for the tools is unfortunately missing. It looks like these would've been found under `Source\P5\Tools`, which is where Ogier (their in-house level editor) would've been located. Ogier integrated with the engine, however, so little bits for supporting the editor can be found in various other places.

### Other Platforms

There’s limited code that still remains for targeting some older platforms such as the GameCube, PlayStation 2 and even the SEGA Dreamcast, but as mentioned earlier, the bulk of code for other platforms has been removed, unfortunately.

### Sorcery

There is some ancient code in here going back to their first project, Sorcery. This can be found in a number of places, and you can find some commented-out code if you just search explicitly for Sorcery.

Per `Source/P5/Projects/Main/GameClasses/WRPG` it also looks like there's left-over code from not only Sorcery but also their short-lived Quake-like arena game?

### Other

There is a reference to “Riddick HD”, which likely refers to the Dark Athena release of Riddick; this wouldn’t even be announced until May 2007 (and wouldn't get released until 2009), but clearly work was already underway in 2006.

Dates mentioned in the code are in the following format - `YYMMDD`. With this in mind, some code for The Darkness goes back to early 2005.
