# Space Billiards
## Build and run

Install the NEST dependencies described in [NEST.md](NEST.md), then run:

```sh
node Maekfile.js
./dist/game
```

On Windows, run `dist/game.exe`.

## Artwork

Edit the layered Aseprite files in `assets/` and export the matching PNGs. Run `node Maekfile.js :assets` to copy the runtime images and artwork credits into `dist/`. Restart the game to see updated images.
This game uses [NEST](NEST.md) for its window, rendering, font, and PNG support.
