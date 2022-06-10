import type Level from '$lib/level'

export default class Scene {
	private previousTime: number | null = null
	private frame: number | null = null

	constructor(
		private readonly canvas: HTMLCanvasElement,
		private readonly context: CanvasRenderingContext2D,
		level: Level
	) {
		console.log('scene created', level)
		this.frame = requestAnimationFrame(this.tick)
	}

	private readonly tick = (currentTime: number) => {
		this.frame = null

		currentTime /= 1000

		const delta = currentTime - (this.previousTime || currentTime)
		this.previousTime = currentTime

		this.frame = requestAnimationFrame(this.tick)
	}

	readonly reset = () => {
		console.log('scene reset')
	}

	readonly clear = () => {
		console.log('scene cleared')
	}

	readonly destroy = () => {
		console.log('scene destroyed')
		if (this.frame) cancelAnimationFrame(this.frame)
	}
}
