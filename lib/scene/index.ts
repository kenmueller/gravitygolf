import type { Unsubscriber } from 'svelte/store'

import type SceneEvents from './events'
import type View from '$lib/view'
import type Position from '$lib/position'
import type Level from '$lib/level'
import type Force from './force'
import type Ball from './ball'
import type Hole from './hole'
import type Star from './star'
import type Wall from './wall'
import type LevelDone from './done'
import EventDispatcher from '$lib/event/dispatcher'
import forceRadius from './force/radius'
import { MAX_FORCE_HIT_DISTANCE, MAX_FORCE_HIT_VELOCITY } from './force/hit'
import FORCE_DELETE_DIMENSIONS from './force/delete/dimensions'
import MAX_STARS from './star/max'
import view from '$lib/view/store'
import distance from '../distance'
import clear from './clear'
import normalizePoint from '../normalize/point'
import normalizeShape from '../normalize/shape'
import collision from './collision'
import scale from '../transform/scale'
import resize from '../transform/resize'
import distanceSquared from '../distance/squared'
import splitHypotenuse from '../split/hypotenuse'
import clamp from './clamp'
import useImage from '$lib/image/use'
import cursorHandler from '$lib/cursor/handler'
import showOverlay from '$lib/overlay/show'
import LevelWin from '../../components/Overlay/LevelWin.svelte'

import gravityImage from '../../images/gravity.png'
import fixedGravityImage from '../../images/fixed-gravity.png'
import antigravityImage from '../../images/antigravity.png'
import fixedAntigravityImage from '../../images/fixed-antigravity.png'
import ballImage from '../../images/ball.png'
import holeImage from '../../images/hole.png'
import starImage from '../../images/star.png'

export default class Scene extends EventDispatcher<SceneEvents> {
	private view: View = undefined as never
	private unsubscribeView: Unsubscriber

	private previousTime: number | null = null
	private frame: number | null = null
	private readonly fixedFramerateDelta: number = 1/60;

	private center = { x: 0, y: 0 }

	private mouseStart: (Position & { button: number }) | null = null
	private mouseCurrent: (Position & { force: Force | null }) | null = null

	private hit = false

	private forces: Force[] = undefined as never
	private ball: Ball = undefined as never
	private hole: Hole = undefined as never
	private stars: Star[] = undefined as never
	private walls: Wall[] = undefined as never

	constructor(
		private readonly canvas: HTMLCanvasElement,
		private readonly context: CanvasRenderingContext2D,
		private readonly level: Level,
		private readonly done: LevelDone
	) {
		super()

		this.unsubscribeView = view.subscribe($view => {
			const initial = !this.view
			this.view = $view as View

			if (initial) this.scale()
			this.resize()
		})

		this.hole = { ...level.hole, image: useImage(holeImage) }

		this.stars = level.stars.map(star => ({
			...star,
			hit: false,
			image: useImage(starImage)
		}))

		this.walls = level.walls

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

		this.frame = requestAnimationFrame(this.tickWrapper)
	}

	private readonly scale = () => scale(this.context, this.view)
	private readonly resize = () => resize(this.canvas, this.view)

	private readonly tickWrapper = (currentTime: number) => {
		if (this.previousTime === null) {
			this.previousTime = 0
		}

		currentTime /= 1000
		let deltaTime = currentTime - this.previousTime;


		while (deltaTime > this.fixedFramerateDelta) { // Repeat until missed ticks caught up
			console.log(this.frame)
			if (this.tick()) {
				return
			}
			deltaTime = currentTime - this.previousTime;
			this.previousTime = currentTime
		}

		this.frame = requestAnimationFrame(this.tickWrapper)
	}

	private readonly tick = () => {
		this.frame = null

		const delta = this.fixedFramerateDelta;

		if (this.hit) {
			if (
				distance(
					normalizePoint(this.ball, this.canvas, this.center),
					normalizePoint(this.hole, this.canvas, this.center)
				) <=
				this.hole.radius - this.ball.radius
			) {
				const stars = this.starCount
				this.done.setStars(stars)

				this.dispatchEvent('win')

				showOverlay(LevelWin, {
					stars,
					hasNext: this.done.hasNext,
					next: this.done.next
				})

				if (this.frame) cancelAnimationFrame(this.frame)
				this.frame = null

				return true
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
		return false
	}

	private readonly down = cursorHandler((cursor, event) => {
		event.preventDefault()

		const mouse = {
			x: Math.floor(cursor.x * this.view.scale),
			y: Math.floor(cursor.y * this.view.scale)
		}

		this.mouseStart = { x: mouse.x, y: mouse.y, button: cursor.button }

		const force = this.hit
			? null
			: this.forces.find(
					force => !force.fixed && this.mouseOnForce(mouse, force)
			  ) ?? null

		this.mouseCurrent = { ...mouse, force }

		if (this.mouseStart.button === 0 && force)
			this.dispatchEvent('force', force)
	})

	private readonly move = cursorHandler(cursor => {
		const mouse = {
			x: Math.floor(cursor.x * this.view.scale),
			y: Math.floor(cursor.y * this.view.scale)
		}

		if (this.mouseCurrent) {
			if (this.mouseStart?.button === 0) {
				if (this.mouseCurrent.force) {
					// Dragging force

					this.mouseCurrent.force.x += mouse.x - this.mouseCurrent.x
					this.mouseCurrent.force.y -= mouse.y - this.mouseCurrent.y
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

				this.dispatchEvent('attempt')

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
			} else if (this.mouseStart.button === 0 && this.mouseCurrent?.force) {
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
				this.mouseCurrent?.force &&
				this.mouseOnForce(this.mouseCurrent, this.mouseCurrent.force)
			) {
				// Delete force

				if (this.deleteForce(this.mouseCurrent.force)) {
					this.dispatchForces()
					this.updateCursor(this.mouseCurrent)
				}
			}

			if (this.mouseStart.button === 0 && this.mouseCurrent?.force)
				this.dispatchEvent('force', null)
		}

		this.mouseStart = this.mouseCurrent = null
	}

	private readonly rightClick = (event: MouseEvent) => {
		event.preventDefault()
	}

	private readonly mouseOnForce = (mouse: Position, force: Force) =>
		distance(normalizePoint(force, this.canvas, this.center), mouse) <=
		forceRadius(this.view.mobile)

	private readonly updateCursor = (mouse: Position) => {
		this.canvas.style.cursor =
			!this.hit &&
			this.forces.some(force => !force.fixed && this.mouseOnForce(mouse, force))
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
					if (!force.fixed) remaining[force.direction === 1 ? 0 : 1]++
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
			fixed: false,
			image: useImage(direction === 1 ? gravityImage : antigravityImage)
		})

		this.dispatchForces()
		this.canvas.style.cursor = 'move'
	}

	readonly reset = (initial = false) => {
		this.hit = false
		this.dispatchEvent('hit', false)

		this.ball = {
			...this.level.ball,
			vx: 0,
			vy: 0,
			image: useImage(ballImage)
		}

		for (const star of this.stars) star.hit = false
		if (!initial) this.dispatchStars()
	}

	readonly clear = (initial = false) => {
		this.reset(initial)

		this.forces = [
			...this.level.fixedGravity.map<Force>(force => ({
				...force,
				direction: 1,
				fixed: true,
				image: useImage(fixedGravityImage)
			})),
			...this.level.fixedAntigravity.map<Force>(force => ({
				...force,
				direction: -1,
				fixed: true,
				image: useImage(fixedAntigravityImage)
			}))
		]

		this.dispatchForces()
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
