A markdown parser that produces a equivalent HTML document.

### Support
It currently supports following HTML tags:

- `#`     ->  `<h1>`
- `##`    ->  `<h2>`
- `###`   ->  `<h3>`
- `-`     ->  `<li>`
- `![]()` ->  `<img>`
- `>`     ->  `<blockquote>`

## Formatting

While flushing the output, it takes care of formatting the HTML.
Currently, the code works but it's not "smart".

## Example

### Markdown
```md
# Heading 1
content 1

## Heading 2
content 2

### Heading 3
content 3
```

### HTML
```html
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Document</title>
</head>
<body>
	<h1>Heading 1</h1>
	content 1<br>
	<br>
	<h2>Heading 2</h2>
	content 2<br>
	<br>
	<h3>Heading 3</h3>
	content 3<br>
</body>
</html>
```

## Build

```console
$ mkdir build && cd build
```

```console
$ cmake .. -DCMAKE_INSTALL_PREFIX=/usr
```

```console
$ make
```
