# About

This is like the music programming language and it's pretty simple and it was supposed to be similar to using tracker. Oh and it's in very early development. And was tested only on linux.

## How

Compilation.

```bash
./build.sh
```

Hello world?

```trang
sample = load("yosample.wav")
pattern = {
sample

sample
sample
}
add_to_sequence(pattern)
```

Run.

```bash
./bin/trang yofile.trang
```

That should generate `out.wav` file in current directory.

## What is supported?

Whatever is in the hello world plus setting bpm through the function `set_bpm()`. You can also add multiple patterns to sequence if you separate them by comma and that's pretty much it I think (for now).

### More about music blocks

Everything inside curly braces is music block. Each line corresponds to 1/16th note (will be configurable in the future) and you can have multiple samples on the same line. Also you should start writing on the newline after `{` as in the hello world example.
