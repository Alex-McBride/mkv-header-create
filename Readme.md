# MKV Header Create

This is a small program that will generate a header and a set of cues if you have a bunch of Matroska clusters, allowing the clusters to be played as a video.

It was created as part of a prototype to serve Matroska video from a proprietary body worn video camera. The video camera would write the frames out in Matroska "clusters". These clusters contained the frame data, but aren't playable on their own due to the lack of header to tell the player what format the data is in, and other such parameters. It also generates cues, which allows seeking in the final video.

This approach wasn't taken in the final solution, and contains no proprietary information, so I was allowed to release it as a standalone program.

This program might be useful if you've got some matroska video that has a damaged header or damaged/missing cues, and you would like to repair it. 