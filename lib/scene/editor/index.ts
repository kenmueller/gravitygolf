import type { Unsubscriber } from 'svelte/store'

import type EditorEvents from './events'
import type View from '$lib/view'
import type Position from '$lib/position'
import type Force from '../force'
import type Ball from '../ball'
import type Hole from '../hole'
import type Star from '../star'
import type Wall from '../wall'
import EventDispatcher from '$lib/event/dispatcher'
import forceRadius from '../force/radius'
import { MAX_FORCE_HIT_DISTANCE, MAX_FORCE_HIT_VELOCITY } from '../force/hit'
import FORCE_DELETE_DIMENSIONS from '../force/delete/dimensions'
import MAX_STARS from '../star/max'
import view from '$lib/view/store'
import distance from '../../distance'
import clear from '../clear'
import normalizePoint from '../../normalize/point'
import normalizeShape from '../../normalize/shape'
import collision from '../collision'
import scale from '../../transform/scale'
import resize from '../../transform/resize'
import distanceSquared from '../../distance/squared'
import splitHypotenuse from '../../split/hypotenuse'
import clamp from '../clamp'
import useImage from '$lib/image/use'
import cursorHandler from '$lib/cursor/handler'

import gravityImage from '../../../images/gravity.png'
import antigravityImage from '../../../images/antigravity.png'
import ballImage from '../../../images/ball.png'
import holeImage from '../../../images/hole.png'
import starImage from '../../../images/star.png'

const BALL_RADIUS = 30
const HOLE_RADIUS = 60
const STAR_RADIUS = 35
const DEFAULT_WALL_SIZE = { width: 40, height: 70 }

export default class EditorScene extends EventDispatcher<EditorEvents> {
	private view: View = undefined as never
	private unsubscribeView: Unsubscriber

	private previousTime: number | null = null
	private frame: number | null = null

	private center = { x: 0, y: 0 }

	private mouseStart: (Position & { button: number }) | null = null
	private mouseCurrent:
		| (Position & { force: Force })
		| (Position & { star: Star })
		| (Position & { ball: true })
		| (Position & { hole: true })
		| (Position & { wall: Wall })
		| Position
		| null = null

	private hit = false

	private forces: Force[] = undefined as never
	private ball: Ball = undefined as never
	private hole: Hole = undefined as never
	private stars: Star[] = undefined as never
	private walls: Wall[] = undefined as never

	constructor(
		private readonly canvas: HTMLCanvasElement,
		private readonly context: CanvasRenderingContext2D
	) {
		super()

		this.unsubscribeView = view.subscribe($view => {
			const initial = !this.view
			this.view = $view as View

			if (initial) this.scale()
			this.resize()
		})

		this.hole = {
			x: 100,
			y: 0,
			radius: HOLE_RADIUS,
			image: useImage(holeImage)
		}

		this.stars = []

		this.walls = []

		this.clear(true)

		document.addEventListener('keydown', this.key)

		if (this.view.mobile) {
			this.canvas.addEventListener('touchstart', this.down)
			this.canvas.addEventListener('touchmove', this.move)
			this.canvas.addEventListener('touchend', this.up)
		} else {
			this.canvas.addEventListener('mousedown', this.down)
			this.canvas.addEventListener('mousemove', this.move)
			this.canvas.addEventListener('mouseup', this.up)
		}

		this.canvas.addEventListener('contextmenu', this.rightClick)

		this.frame = requestAnimationFrame(this.tick)
	}

	private readonly scale = () => scale(this.context, this.view)
	private readonly resize = () => resize(this.canvas, this.view)

	private readonly tick = (currentTime: number) => {
		this.frame = null

		currentTime /= 1000

		const delta = currentTime - (this.previousTime || currentTime)
		this.previousTime = currentTime

		if (this.hit) {
			if (
				distance(
					normalizePoint(this.ball, this.canvas, this.center),
					normalizePoint(this.hole, this.canvas, this.center)
				) <=
				this.hole.radius - this.ball.radius
			) {
				const stars = this.starCount

				alert(`Congratulations! You got ${stars} star${stars === 1 ? '' : 's'}`)

				this.reset()
			}

			for (const wall of this.walls) {
				const angle = collision(
					normalizePoint(this.ball, this.canvas, this.center),
					normalizeShape(wall, this.canvas, this.center)
				)

				if (angle !== null) {
					const v = Math.atan2(-this.ball.vy, this.ball.vx)

					const bounceAngle = 2 * angle - v + Math.PI
					const speed = Math.sqrt(this.ball.vy ** 2 + this.ball.vx ** 2)

					this.ball.vy = -Math.sin(bounceAngle) * speed
					this.ball.vx = Math.cos(bounceAngle) * speed
				}
			}

			for (const force of this.forces) {
				const rSquared = distanceSquared(
					normalizePoint(this.ball, this.canvas, this.center),
					normalizePoint(force, this.canvas, this.center)
				)

				const { x, y } = splitHypotenuse(
					normalizePoint(force, this.canvas, this.center),
					normalizePoint(this.ball, this.canvas, this.center),
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

		const normalizedBall = normalizeShape(this.ball, this.canvas, this.center)
		const normalizedHole = normalizeShape(this.hole, this.canvas, this.center)

		// walls

		for (const wall of this.walls) {
			const { x, y, width, height } = normalizeShape(
				wall,
				this.canvas,
				this.center
			)

			this.context.fillStyle = 'white'
			this.context.fillRect(x, y, width, height)
		}

		// hole

		if (this.hole.image.current)
			this.context.drawImage(
				this.hole.image.current,
				normalizedHole.x,
				normalizedHole.y,
				normalizedHole.radius * 2,
				normalizedHole.radius * 2
			)

		// stars

		for (const star of this.stars) {
			if (!star.image.current) continue

			const normalizedStar = normalizeShape(star, this.canvas, this.center)

			if (
				!star.hit &&
				distance(normalizedBall, normalizedStar) <=
					normalizedBall.radius + normalizedStar.radius
			) {
				star.hit = true
				this.dispatchStars()
			}

			if (star.hit) this.context.globalAlpha = 0.5

			this.context.drawImage(
				star.image.current,
				normalizedStar.x,
				normalizedStar.y,
				normalizedStar.radius * 2,
				normalizedStar.radius * 2
			)

			if (star.hit) this.context.globalAlpha = 1
		}

		// forces

		for (const force of this.forces) {
			if (!force.image.current) continue

			const { x, y, radius } = normalizeShape(
				{ ...force, radius: forceRadius(this.view.mobile) },
				this.canvas,
				this.center
			)

			this.context.drawImage(force.image.current, x, y, radius * 2, radius * 2)
		}

		// ball

		if (this.ball.image.current)
			this.context.drawImage(
				this.ball.image.current,
				normalizedBall.x,
				normalizedBall.y,
				normalizedBall.radius * 2,
				normalizedBall.radius * 2
			)

		this.frame = requestAnimationFrame(this.tick)
	}

	private readonly down = cursorHandler((cursor, event) => {
		event.preventDefault()
		if (!this.hit) {
			const mouse = {
				x: Math.floor(cursor.x * this.view.scale),
				y: Math.floor(cursor.y * this.view.scale)
			}

			this.mouseStart = { x: mouse.x, y: mouse.y, button: cursor.button }

			const force = this.forces.find(force => this.mouseOnForce(mouse, force))
			if (force) {
				this.mouseCurrent = { ...mouse, force }
				if (this.mouseStart.button === 0) this.dispatchEvent('force', force)
				return
			}
			const star = this.stars.find(star => this.mouseOnStar(mouse, star))
			if (star) {
				this.mouseCurrent = { ...mouse, star }
				if (this.mouseStart.button === 0) this.dispatchEvent('star', star)
				return
			}
			const wall = this.walls.find(wall => this.mouseOnWall(mouse, wall))
			if (wall) {
				this.mouseCurrent = { ...mouse, wall }
				if (this.mouseStart.button === 0) this.dispatchEvent('wall', wall)
				return
			}
			if (this.mouseOnBall(mouse)) {
				this.mouseCurrent = { ...mouse, ball: true }
				return
			}
			if (this.mouseOnHole(mouse)) {
				this.mouseCurrent = { ...mouse, hole: true }
				return
			}
			this.mouseCurrent = mouse
		}
	})

	private readonly move = cursorHandler(cursor => {
		const mouse = {
			x: Math.floor(cursor.x * this.view.scale),
			y: Math.floor(cursor.y * this.view.scale)
		}

		if (this.mouseCurrent) {
			if (this.mouseStart?.button === 0) {
				if ('force' in this.mouseCurrent) {
					// Dragging force

					this.mouseCurrent.force.x += mouse.x - this.mouseCurrent.x
					this.mouseCurrent.force.y -= mouse.y - this.mouseCurrent.y
				} else if ('star' in this.mouseCurrent) {
					// Dragging star
					this.mouseCurrent.star.x += mouse.x - this.mouseCurrent.x
					this.mouseCurrent.star.y -= mouse.y - this.mouseCurrent.y
				} else if ('wall' in this.mouseCurrent) {
					// Dragging star
					this.mouseCurrent.wall.x += mouse.x - this.mouseCurrent.x
					this.mouseCurrent.wall.y -= mouse.y - this.mouseCurrent.y
				} else if ('ball' in this.mouseCurrent) {
					// Dragging star
					this.ball.x += mouse.x - this.mouseCurrent.x
					this.ball.y -= mouse.y - this.mouseCurrent.y
				} else if ('hole' in this.mouseCurrent) {
					// Dragging star
					this.hole.x += mouse.x - this.mouseCurrent.x
					this.hole.y -= mouse.y - this.mouseCurrent.y
				} else {
					// Panning

					this.center.x += mouse.x - this.mouseCurrent.x
					this.center.y -= mouse.y - this.mouseCurrent.y
				}
			}

			this.mouseCurrent.x = mouse.x
			this.mouseCurrent.y = mouse.y
		}

		this.updateCursor(mouse)
	})

	private readonly up = () => {
		if (!(this.mouseStart && this.mouseCurrent)) return

		if (!this.hit) {
			if (
				this.mouseStart.button === 0 &&
				this.mouseCurrent.x === this.mouseStart.x &&
				this.mouseCurrent.y === this.mouseStart.y
			) {
				// Hit the ball

				this.hit = true
				this.dispatchEvent('hit', true)

				const normalizedBall = normalizePoint(
					this.ball,
					this.canvas,
					this.center
				)

				const distanceFactor = distance(this.mouseCurrent, normalizedBall)

				const { x, y } = splitHypotenuse(
					this.mouseCurrent,
					normalizedBall,
					distanceFactor,
					Math.min(distanceFactor / MAX_FORCE_HIT_DISTANCE, 1) *
						MAX_FORCE_HIT_VELOCITY
				)

				this.ball.vx = x
				this.ball.vy = y
			} else if (this.mouseStart.button === 0 && 'force' in this.mouseCurrent) {
				// End dragging force

				if (
					this.mouseCurrent.x >
						this.canvas.width -
							FORCE_DELETE_DIMENSIONS.width * this.view.scale &&
					this.mouseCurrent.y >
						this.canvas.height -
							FORCE_DELETE_DIMENSIONS.height * this.view.scale &&
					this.deleteForce(this.mouseCurrent.force)
				) {
					this.dispatchForces()
					this.updateCursor(this.mouseCurrent)
				}
			} else if (
				this.mouseStart.button === 2 &&
				'force' in this.mouseCurrent &&
				this.mouseOnForce(this.mouseCurrent, this.mouseCurrent.force)
			) {
				// Delete force

				if (this.deleteForce(this.mouseCurrent.force)) {
					this.dispatchForces()
					this.updateCursor(this.mouseCurrent)
				}
			}

			if (this.mouseStart.button === 0 && 'force' in this.mouseCurrent)
				this.dispatchEvent('force', null)
			else if (this.mouseStart.button === 0 && 'star' in this.mouseCurrent)
				this.dispatchEvent('star', null)
		}

		this.mouseStart = this.mouseCurrent = null
	}

	private readonly rightClick = (event: MouseEvent) => {
		event.preventDefault()
	}

	private readonly mouseOnForce = (mouse: Position, force: Force) =>
		distance(normalizePoint(force, this.canvas, this.center), mouse) <=
		forceRadius(this.view.mobile)

	private readonly mouseOnStar = (mouse: Position, star: Star) =>
		distance(normalizePoint(star, this.canvas, this.center), mouse) <=
		STAR_RADIUS

	private readonly mouseOnWall = (mouse: Position, wall: Wall) =>
		distance(normalizePoint(wall, this.canvas, this.center), mouse) <=
		STAR_RADIUS

	private readonly mouseOnBall = (mouse: Position) =>
		distance(normalizePoint(this.ball, this.canvas, this.center), mouse) <=
		BALL_RADIUS

	private readonly mouseOnHole = (mouse: Position) =>
		distance(normalizePoint(this.hole, this.canvas, this.center), mouse) <=
		HOLE_RADIUS

	private readonly updateCursor = (mouse: Position) => {
		this.canvas.style.cursor =
			!this.hit &&
			(this.forces.some(force => this.mouseOnForce(mouse, force)) ||
				this.stars.some(star => this.mouseOnStar(mouse, star)) ||
				this.walls.some(wall => this.mouseOnWall(mouse, wall)) ||
				this.mouseOnBall(mouse) ||
				this.mouseOnHole(mouse))
				? 'move'
				: ''
	}

	private readonly key = ({ key }: KeyboardEvent) => {
		if (key === ' ') this.reset()
	}

	private readonly deleteForce = (force: Force) => {
		const index = this.forces.indexOf(force)
		const found = index >= 0

		if (found) this.forces.splice(index, 1)
		return found
	}

	private readonly dispatchForces = () => {
		this.dispatchEvent(
			'forces',
			...this.forces.reduce<[number, number]>(
				(remaining, force) => {
					remaining[force.direction === 1 ? 0 : 1]++
					return remaining
				},
				[0, 0]
			)
		)
	}

	private get starCount() {
		return (
			MAX_STARS -
			this.stars.length +
			this.stars.reduce<number>((stars, { hit }) => stars + (hit ? 1 : 0), 0)
		)
	}

	private readonly dispatchStars = () => {
		this.dispatchEvent('stars', this.starCount)
	}

	readonly addForce = ({ x, y }: Position, direction: 1 | -1) => {
		this.forces.push({
			x:
				x * this.view.scale +
				forceRadius(this.view.mobile) -
				this.canvas.width / 2 -
				this.center.x,
			y:
				-y * this.view.scale -
				forceRadius(this.view.mobile) +
				this.canvas.height / 2 -
				this.center.y,
			direction,
			image: useImage(direction === 1 ? gravityImage : antigravityImage)
		})

		this.dispatchForces()
		this.canvas.style.cursor = 'move'
	}

	readonly addObstacle = ({ x, y }: Position) => {
		this.walls.push({
			x: x * this.view.scale + 10 - this.canvas.width / 2 - this.center.x,
			y: -y * this.view.scale - 17 + this.canvas.height / 2 - this.center.y,
			width: DEFAULT_WALL_SIZE.width * this.view.scale,
			height: DEFAULT_WALL_SIZE.height * this.view.scale
		})

		this.canvas.style.cursor = 'move'
	}

	readonly addStar = ({ x, y }: Position) => {
		this.stars.push({
			x: x * this.view.scale + 35 - this.canvas.width / 2 - this.center.x,
			y: -y * this.view.scale - 35 + this.canvas.height / 2 - this.center.y,
			radius: STAR_RADIUS,
			hit: false,
			image: useImage(starImage)
		})
		this.dispatchEvent('defaultStars', this.stars.length)

		this.canvas.style.cursor = 'move'
	}

	readonly reset = (initial = false) => {
		this.hit = false
		this.dispatchEvent('hit', false)

		this.ball = {
			x: -100,
			y: 0,
			radius: BALL_RADIUS,
			vx: 0,
			vy: 0,
			image: useImage(ballImage)
		}

		for (const star of this.stars) star.hit = false
		if (!initial) this.dispatchStars()
	}

	readonly clear = (initial = false) => {
		this.reset(initial)

		this.forces = []
		this.dispatchForces()

		this.stars = []
		this.dispatchStars()

		this.walls = []
		this.ball.x = -100
		this.ball.y = this.ball.vx = this.ball.vy = 0
		this.hole.x = 100
		this.hole.y = 0

		this.dispatchEvent('clear')
	}

	readonly destroy = () => {
		this.removeAllEventListeners()

		this.unsubscribeView()
		document.removeEventListener('keydown', this.key)

		if (this.view.mobile) {
			this.canvas.removeEventListener('touchstart', this.down)
			this.canvas.removeEventListener('touchmove', this.move)
			this.canvas.removeEventListener('touchend', this.up)
		} else {
			this.canvas.removeEventListener('mousedown', this.down)
			this.canvas.removeEventListener('mousemove', this.move)
			this.canvas.removeEventListener('mouseup', this.up)
		}

		if (this.frame) cancelAnimationFrame(this.frame)
	}
}
