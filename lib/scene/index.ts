import { goto } from '$app/navigation'

import type Level from '$lib/level'
import type Ball from './ball'
import type Hole from './hole'
import type Wall from './wall'
import type Force from './force'
import levels from '$lib/level/levels.json'
import distance from './distance'
import clear from './clear'
import normalizeCircle from './normalize/circle'
import normalizeRectangle from './normalize/rectangle'
import collision from './collision'
import scale from './transform/scale'
import resize from './transform/resize'
import distanceSquared from './distance/squared'
import splitHypotenuse from './split/hypotenuse'
import clamp from './clamp'
import useImage from '$lib/image/use'

import ballImage from '../../images/ball.png'
import holeImage from '../../images/ball.png'
import gravityImage from '../../images/ball.png'
import antigravityImage from '../../images/ball.png'

const FORCE_RADIUS = 30

const MAX_DISTANCE = 800
const MAX_VELOCITY = 500

export default class Scene {
	private previousTime: number | null = null
	private frame: number | null = null

	private mouseStart: { x: number; y: number } | null = null
	private mouseCurrent: { x: number; y: number; force: Force } | null = null

	private hit = false

	private ball: Ball = undefined as never
	private hole: Hole = undefined as never
	private walls: Wall[] = undefined as never
	private forces: Force[] = undefined as never

	constructor(
		private readonly canvas: HTMLCanvasElement,
		private readonly context: CanvasRenderingContext2D,
		private readonly level: Level & { id: number }
	) {
		this.clear()

		this.hole = {
			x: this.level.hole[0],
			y: this.level.hole[1],
			radius: this.level.hole[2],
			image: useImage(holeImage)
		}

		this.walls = this.level.walls.map(wall => ({
			x: wall[0],
			y: wall[1],
			width: wall[2],
			height: wall[3]
		}))

		this.scale()

		this.resize()
		window.addEventListener('resize', this.resize)

		document.addEventListener('keydown', this.key)

		this.canvas.addEventListener('mousedown', this.down)
		this.canvas.addEventListener('mousemove', this.move)
		this.canvas.addEventListener('mouseup', this.up)

		this.canvas.addEventListener('contextmenu', this.rightClick)

		this.frame = requestAnimationFrame(this.tick)
	}

	private readonly scale = () => scale(this.context)
	private readonly resize = () => resize(this.canvas)

	private readonly tick = (currentTime: number) => {
		this.frame = null

		currentTime /= 1000

		const delta = currentTime - (this.previousTime || currentTime)
		this.previousTime = currentTime

		if (this.hit) {
			if (
				distance(
					normalizeCircle(this.ball, this.canvas),
					normalizeCircle(this.hole, this.canvas)
				) <=
				this.hole.radius - this.ball.radius
			) {
				alert('Congratulations!')

				const lastLevel = this.level.id === levels.length
				const suffix = lastLevel ? '' : `/${this.level.id + 1}`

				goto(`/levels${suffix}`).catch(({ message }) => alert(message))

				return
			}

			for (const wall of this.walls) {
				const angle = collision(
					normalizeCircle(this.ball, this.canvas),
					normalizeRectangle(wall, this.canvas)
				)

				if (angle !== null) {
					const v = Math.atan2(-this.ball.vy, this.ball.vx)

					const bounceAngle = 2 * angle - v + Math.PI
					const speed = Math.sqrt(this.ball.vy ** 2 + this.ball.vx ** 2)

					this.ball.vy = -Math.sin(bounceAngle) * speed
					this.ball.vx = Math.cos(bounceAngle) * speed

					// console.log('hit', angle, v, bounceAngle, speed, this.ball)
					// return
				}
			}

			for (const force of this.forces) {
				const rSquared = distanceSquared(
					normalizeCircle(this.ball, this.canvas),
					normalizeCircle(force, this.canvas)
				)

				const { x, y } = splitHypotenuse(
					normalizeCircle(force, this.canvas),
					normalizeCircle(this.ball, this.canvas),
					Math.sqrt(rSquared),
					clamp(-5000, 5000, (force.direction * 100000000) / rSquared)
				)

				this.ball.vx += x * delta
				this.ball.vy += y * delta
			}
		}

		this.ball.x += this.ball.vx * delta
		this.ball.y -= this.ball.vy * delta

		clear(this.canvas, this.context)

		// ball

		const normalizedBall = normalizeCircle(this.ball, this.canvas)

		if (this.ball.image.current)
			this.context.drawImage(
				this.ball.image.current,
				normalizedBall.x - this.ball.radius,
				normalizedBall.y - this.ball.radius,
				this.ball.radius * 2,
				this.ball.radius * 2
			)

		// hole

		const normalizedHole = normalizeCircle(this.hole, this.canvas)

		if (this.hole.image.current)
			this.context.drawImage(
				this.hole.image.current,
				normalizedHole.x - this.hole.radius,
				normalizedHole.y - this.hole.radius,
				this.hole.radius * 2,
				this.hole.radius * 2
			)

		// walls

		for (const wall of this.walls) {
			const { x, y } = normalizeRectangle(wall, this.canvas)

			this.context.fillStyle = 'white'
			this.context.fillRect(x, y, wall.width, wall.height)
		}

		// forces

		for (const force of this.forces) {
			if (!force.image.current) continue

			const { x, y } = normalizeCircle(force, this.canvas)

			this.context.drawImage(
				force.image.current,
				x - FORCE_RADIUS,
				y - FORCE_RADIUS,
				FORCE_RADIUS * 2,
				FORCE_RADIUS * 2
			)
		}

		this.frame = requestAnimationFrame(this.tick)
	}

	private readonly down = ({ offsetX, offsetY }: MouseEvent) => {
		if (this.hit) return

		const scale = window.devicePixelRatio

		const mouse = (this.mouseStart = {
			x: Math.floor(offsetX * scale),
			y: Math.floor(offsetY * scale)
		})

		const force = this.forces.find(
			force =>
				distance(normalizeCircle(force, this.canvas), mouse) <= FORCE_RADIUS
		)

		if (force) this.mouseCurrent = { ...mouse, force }
	}

	private readonly move = ({ offsetX, offsetY }: MouseEvent) => {
		if (this.hit) return

		const scale = window.devicePixelRatio

		const mouse = {
			x: Math.floor(offsetX * scale),
			y: Math.floor(offsetY * scale)
		}

		if (this.mouseCurrent) {
			this.mouseCurrent.force.x += mouse.x - this.mouseCurrent.x
			this.mouseCurrent.force.y -= mouse.y - this.mouseCurrent.y

			this.mouseCurrent.x = mouse.x
			this.mouseCurrent.y = mouse.y
		}

		this.updateCursor(mouse)
	}

	private readonly up = ({ offsetX, offsetY, button }: MouseEvent) => {
		if (this.hit || !this.mouseStart) return

		const scale = window.devicePixelRatio

		const mouse = {
			x: Math.floor(offsetX * scale),
			y: Math.floor(offsetY * scale)
		}

		if (button === 2) {
			const index = this.mouseCurrent
				? this.forces.indexOf(this.mouseCurrent.force)
				: -1

			if (index >= 0) {
				this.forces.splice(index, 1)
				this.updateCursor(mouse)
			}
		} else if (mouse.x === this.mouseStart.x && mouse.y === this.mouseStart.y) {
			this.hit = true

			const normalizedBall = normalizeCircle(this.ball, this.canvas)
			const distanceFactor = distance(mouse, normalizedBall)

			const { x, y } = splitHypotenuse(
				mouse,
				normalizedBall,
				distanceFactor,
				Math.min(distanceFactor / MAX_DISTANCE, 1) * MAX_VELOCITY
			)

			this.ball.vx = x
			this.ball.vy = y
		}

		this.mouseStart = this.mouseCurrent = null
	}

	private readonly rightClick = (event: MouseEvent) => {
		event.preventDefault()
	}

	private readonly updateCursor = (mouse: { x: number; y: number }) => {
		this.canvas.style.cursor = this.forces.some(
			force =>
				distance(normalizeCircle(force, this.canvas), mouse) <= FORCE_RADIUS
		)
			? 'move'
			: ''
	}

	private readonly key = ({ key }: KeyboardEvent) => {
		if (key === ' ') this.reset()
	}

	readonly reset = () => {
		this.hit = false

		this.ball = {
			x: this.level.ball[0],
			y: this.level.ball[1],
			radius: this.level.ball[2],
			vx: 0,
			vy: 0,
			image: useImage(ballImage)
		}
	}

	readonly clear = () => {
		this.reset()
		this.forces = [
			{ x: -100, y: 0, direction: 1, image: useImage(gravityImage) }
		]
	}

	readonly destroy = () => {
		window.removeEventListener('resize', this.resize)
		document.removeEventListener('keydown', this.key)

		this.canvas.removeEventListener('mousedown', this.down)
		this.canvas.removeEventListener('mousemove', this.move)
		this.canvas.removeEventListener('mouseup', this.up)

		if (this.frame) cancelAnimationFrame(this.frame)
	}
}
