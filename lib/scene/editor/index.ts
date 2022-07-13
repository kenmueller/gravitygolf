import type { Unsubscriber } from 'svelte/store'

import type RawLevel from '$lib/level/raw'
import type EditorEvents from './events'
import type View from '$lib/view'
import type Position from '$lib/position'
import type Force from '../force'
import type Ball from '../ball'
import type Hole from '../hole'
import type Star from '../star'
import type ResizableWall from '../wall/resizable'
import type WallCorner from '../wall/corner'
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
import showOverlay from '$lib/overlay/show'
import EditorWin from '../../../components/Overlay/EditorWin.svelte'

import gravityImage from '../../../images/gravity.png'
import antigravityImage from '../../../images/antigravity.png'
import ballImage from '../../../images/ball.png'
import holeImage from '../../../images/hole.png'
import starImage from '../../../images/star.png'

const BALL_RADIUS = 30
const HOLE_RADIUS = 60
const STAR_RADIUS = 35
const WALL_CORNER_RADIUS = 10
const WALL_CORNER_EXTRA = WALL_CORNER_RADIUS / 3
const WALL_CORNER_COLOR = 'rgb(127, 127, 255)'
const DEFAULT_WALL_SIZE = { width: 40, height: 70 }

export default class EditorScene extends EventDispatcher<EditorEvents> {
	private view: View = undefined as never
	private unsubscribeView: Unsubscriber

	private previousTime: number | null = null
	private frame: number | null = null

	private center = { x: 0, y: 0 }

	private defaultBallPosition: Position = { x: -100, y: 0 }

	private mouseStart: (Position & { button: number }) | null = null
	private mouseCurrent:
		| (Position & { force: Force })
		| (Position & { star: Star })
		| (Position & { ball: true })
		| (Position & { hole: true })
		| (Position & { wall: ResizableWall })
		| (Position & { wallCorner: WallCorner })
		| Position
		| null = null

	private hit = false

	private forces: Force[] = undefined as never
	private ball: Ball = undefined as never
	private hole: Hole = undefined as never
	private stars: Star[] = undefined as never
	private walls: ResizableWall[] = undefined as never
	private neswWallCorners: WallCorner[] = undefined as never
	private nwseWallCorners: WallCorner[] = undefined as never

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
		this.neswWallCorners = []
		this.nwseWallCorners = []

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
				showOverlay(EditorWin, {
					stars: this.starCount,
					data: () => this.data,
					reset: () => {
						this.reset()
						this.frame = requestAnimationFrame(this.tick)
					}
				})

				if (this.frame) cancelAnimationFrame(this.frame)
				this.frame = null

				return
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
		if (!this.hit)
			for (const corner of [...this.neswWallCorners, ...this.nwseWallCorners]) {
				const shape = normalizeShape(
					{ ...corner, radius: WALL_CORNER_RADIUS },
					this.canvas,
					this.center
				)

				shape.x += shape.radius
				shape.y += shape.radius

				this.context.beginPath()

				this.context.fillStyle = WALL_CORNER_COLOR
				this.context.globalAlpha = 0.5

				this.context.arc(shape.x, shape.y, shape.radius, 0, 2 * Math.PI)
				this.context.fill()

				this.context.globalAlpha = 1

				this.context.beginPath()

				this.context.lineWidth = 2
				this.context.strokeStyle = WALL_CORNER_COLOR

				this.context.arc(shape.x, shape.y, shape.radius, 0, 2 * Math.PI)
				this.context.stroke()
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

		// center

		const center = normalizePoint({ x: 0, y: 0 }, this.canvas, this.center)

		this.context.lineWidth = 1
		this.context.strokeStyle = 'gray'
		this.context.globalAlpha = 0.5

		this.context.beginPath()
		this.context.moveTo(center.x, 0)
		this.context.lineTo(center.x, this.canvas.height)
		this.context.stroke()

		this.context.beginPath()
		this.context.moveTo(0, center.y)
		this.context.lineTo(this.canvas.width, center.y)
		this.context.stroke()

		this.context.globalAlpha = 1

		this.frame = requestAnimationFrame(this.tick)
	}

	private get data(): RawLevel {
		const forces = this.forceCounts

		return {
			gravity: forces[0],
			antigravity: forces[1],
			ball: [
				this.defaultBallPosition.x,
				this.defaultBallPosition.y,
				this.ball.radius
			],
			hole: [this.hole.x, this.hole.y, this.hole.radius],
			stars: this.stars.map(({ x, y, radius }) => [x, y, radius]),
			walls: this.walls.map(({ x, y, width, height }) => [x, y, width, height])
		}
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
			const wallCorner = [
				...this.neswWallCorners,
				...this.nwseWallCorners
			].find(this.mouseOnWallCorner(mouse))
			if (wallCorner) {
				this.mouseCurrent = { ...mouse, wallCorner }
				return
			}
			const wall = this.walls.find(this.mouseOnWall(mouse))
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
					for (const corner of this.mouseCurrent.wall.corners) {
						corner.x += mouse.x - this.mouseCurrent.x
						corner.y -= mouse.y - this.mouseCurrent.y
					}
				} else if ('wallCorner' in this.mouseCurrent) {
					const corner = this.mouseCurrent.wallCorner
					const diffX = mouse.x - this.mouseCurrent.x
					const diffY = mouse.y - this.mouseCurrent.y
					const wall = corner.wall
					wall.x += diffX / 2
					wall.y -= diffY / 2
					switch (corner.position) {
						case 'NE':
							wall.width += diffX
							wall.height -= diffY
							break
						case 'NW':
							wall.width -= diffX
							wall.height -= diffY
							break
						case 'SE':
							wall.width += diffX
							wall.height += diffY
							break
						case 'SW':
							wall.width -= diffX
							wall.height += diffY
							break
					}
					this.positionWallCorners(wall)
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
				this.dispatchEvent('force', null)
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
			} else if (this.mouseStart.button === 0 && 'star' in this.mouseCurrent) {
				this.dispatchEvent('star', null)
				if (
					this.mouseCurrent.x >
						this.canvas.width -
							FORCE_DELETE_DIMENSIONS.width * this.view.scale &&
					this.mouseCurrent.y >
						this.canvas.height -
							FORCE_DELETE_DIMENSIONS.height * this.view.scale &&
					this.deleteStar(this.mouseCurrent.star)
				) {
					this.dispatchStars()
					this.updateCursor(this.mouseCurrent)
				}
			} else if (this.mouseStart.button === 0 && 'wall' in this.mouseCurrent) {
				this.dispatchEvent('wall', null)
				if (
					this.mouseCurrent.x >
						this.canvas.width -
							FORCE_DELETE_DIMENSIONS.width * this.view.scale &&
					this.mouseCurrent.y >
						this.canvas.height -
							FORCE_DELETE_DIMENSIONS.height * this.view.scale &&
					this.deleteWall(this.mouseCurrent.wall)
				) {
					this.updateCursor(this.mouseCurrent)
				}
			} else if (
				this.mouseStart.button === 0 &&
				'wallCorner' in this.mouseCurrent
			) {
				const wall = this.mouseCurrent.wallCorner.wall
				if (wall.width < 0) wall.width *= -1
				if (wall.height < 0) wall.height *= -1
				this.positionWallCorners(wall)
			} else if (this.mouseStart.button === 0 && 'ball' in this.mouseCurrent) {
				// normalizePoint(corner, this.canvas, this.center)
				this.defaultBallPosition.x = this.mouseCurrent.x
				this.defaultBallPosition.y = this.mouseCurrent.y
				console.log(this.mouseCurrent.x, this.mouseCurrent.y)
			}
		}
		console.log('hi')

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

	private readonly mouseOnWall = (mouse: Position) => (wall: ResizableWall) => {
		const point = normalizePoint(wall, this.canvas, this.center)
		return (
			mouse.x >= point.x - wall.width / 2 &&
			mouse.x <= point.x + wall.width / 2 &&
			mouse.y >= point.y - wall.height / 2 &&
			mouse.y <= point.y + wall.height / 2
		)
	}

	private readonly mouseOnWallCorner =
		(mouse: Position) => (corner: WallCorner) =>
			distance(normalizePoint(corner, this.canvas, this.center), mouse) <=
			WALL_CORNER_RADIUS

	private readonly mouseOnBall = (mouse: Position) =>
		distance(normalizePoint(this.ball, this.canvas, this.center), mouse) <=
		BALL_RADIUS

	private readonly mouseOnHole = (mouse: Position) =>
		distance(normalizePoint(this.hole, this.canvas, this.center), mouse) <=
		HOLE_RADIUS

	private readonly updateCursor = (mouse: Position) => {
		if (!this.hit) {
			if (this.neswWallCorners.some(this.mouseOnWallCorner(mouse)))
				this.canvas.style.cursor = 'nesw-resize'
			else if (this.nwseWallCorners.some(this.mouseOnWallCorner(mouse)))
				this.canvas.style.cursor = 'nwse-resize'
			else if (
				this.forces.some(force => this.mouseOnForce(mouse, force)) ||
				this.stars.some(star => this.mouseOnStar(mouse, star)) ||
				this.walls.some(this.mouseOnWall(mouse)) ||
				this.mouseOnBall(mouse) ||
				this.mouseOnHole(mouse)
			)
				this.canvas.style.cursor = 'move'
			else this.canvas.style.cursor = ''
		}
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

	private readonly deleteStar = (star: Star) => {
		const index = this.stars.indexOf(star)
		const found = index >= 0

		if (found) this.stars.splice(index, 1)
		return found
	}

	private readonly deleteWall = (wall: ResizableWall) => {
		const index = this.walls.indexOf(wall)
		const found = index >= 0

		if (found) this.walls.splice(index, 1)
		return found
	}

	private get forceCounts() {
		return this.forces.reduce<[number, number]>(
			(remaining, force) => {
				remaining[force.direction === 1 ? 0 : 1]++
				return remaining
			},
			[0, 0]
		)
	}

	private readonly dispatchForces = () => {
		this.dispatchEvent('forces', ...this.forceCounts)
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

	private readonly positionWallCorners = (wall: ResizableWall) => {
		for (const wallCorner of wall.corners) {
			switch (wallCorner.position) {
				case 'NE':
					wallCorner.x = wall.x + wall.width / 2 + WALL_CORNER_EXTRA
					wallCorner.y = wall.y + wall.height / 2 + WALL_CORNER_EXTRA
					break
				case 'NW':
					wallCorner.x = wall.x - wall.width / 2 - WALL_CORNER_EXTRA
					wallCorner.y = wall.y + wall.height / 2 + WALL_CORNER_EXTRA
					break
				case 'SE':
					wallCorner.x = wall.x + wall.width / 2 + WALL_CORNER_EXTRA
					wallCorner.y = wall.y - wall.height / 2 - WALL_CORNER_EXTRA
					break
				case 'SW':
					wallCorner.x = wall.x - wall.width / 2 - WALL_CORNER_EXTRA
					wallCorner.y = wall.y - wall.height / 2 - WALL_CORNER_EXTRA
					break
			}
		}
	}

	readonly addObstacle = ({ x, y }: Position) => {
		const wall: ResizableWall = {
			x: x * this.view.scale + 10 - this.canvas.width / 2 - this.center.x,
			y: -y * this.view.scale - 17 + this.canvas.height / 2 - this.center.y,
			width: DEFAULT_WALL_SIZE.width * this.view.scale,
			height: DEFAULT_WALL_SIZE.height * this.view.scale,
			corners: []
		}
		const neCorner: WallCorner = {
			x: wall.x + wall.width / 2 + WALL_CORNER_EXTRA,
			y: wall.y + wall.height / 2 + WALL_CORNER_EXTRA,
			position: 'NE',
			wall
		}
		const nwCorner: WallCorner = {
			x: wall.x - wall.width / 2 - WALL_CORNER_EXTRA,
			y: wall.y + wall.height / 2 + WALL_CORNER_EXTRA,
			position: 'NW',
			wall
		}
		const seCorner: WallCorner = {
			x: wall.x + wall.width / 2 + WALL_CORNER_EXTRA,
			y: wall.y - wall.height / 2 - WALL_CORNER_EXTRA,
			position: 'SE',
			wall
		}
		const swCorner: WallCorner = {
			x: wall.x - wall.width / 2 - WALL_CORNER_EXTRA,
			y: wall.y - wall.height / 2 - WALL_CORNER_EXTRA,
			position: 'SW',
			wall
		}
		wall.corners = [neCorner, nwCorner, seCorner, swCorner]
		this.walls.push(wall)
		this.neswWallCorners.push(neCorner)
		this.neswWallCorners.push(swCorner)
		this.nwseWallCorners.push(nwCorner)
		this.nwseWallCorners.push(seCorner)

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
			x: this.defaultBallPosition.x,
			y: this.defaultBallPosition.y,
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
		this.neswWallCorners = []
		this.nwseWallCorners = []

		this.defaultBallPosition = { x: -100, y: 0 }
		this.ball.x = this.defaultBallPosition.x
		this.ball.y = this.defaultBallPosition.y
		this.ball.vx = this.ball.vy = 0
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
