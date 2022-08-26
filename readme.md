# SVG to CSS Encoder

## URL encoder for SVG: fast tool written in C

[GitHub Repo](https://github.com/maddsua/svg-to-css-encoder)

This code can be compiled for Linux or Mac with `#define PLATFORM_WIN32` commented or deleted.

### Usage

1. Open command line in directory that contains `.svg` files. If you don't specify any input files, on windows, it'll try to find them in current dir
2. Run `csvg`
3. Observe `result.css` appering in that folder. It contains ready to use CSS code
4. Use that code in your product
5. ????
6. Profit!

<img src="info/demo.webp" style="width: 720px;"/>


### Flags

`-i` : input file. Usage: `-i vector.svg`

`-o` : output file. Usage: `-o style.css`

`-p` : adds another CSS class to the image. Usage: `-p vectorImage`

`-s` : enables code optimization (gets rid of metadata)

`-r` : open file in a shell when done


#### CLI:

```
D:\git\svg-to-css-encoder\src>csvg -s -r -o vector.css -p icon

SVG to CSS Encoder build 60

Found 1 svg files

 --> "orig.svg" done

Saved to "vector.css" with a ".icon" class

```

### Results in:

vector.css:

```
.icon.orig {
	background-image: url("data:image/svg+xml,%3C%3Fxml......%2Fsvg%3E");
}
```


## Download

[📦 Download app (exe file)](build/win64-cli-b402.zip)

