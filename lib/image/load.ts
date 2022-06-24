const loadImage = (source: string) =>
	new Promise<HTMLImageElement>((resolve, reject) => {
		const image = new Image()

		image.addEventListener(
			'load',
			() => {
				resolve(image)
			},
			{ once: true }
		)

		image.addEventListener(
			'error',
			({ error }) => {
				reject(error)
			},
			{ once: true }
		)

		image.src = source
	})

export default loadImage
