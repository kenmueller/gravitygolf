import type Position from '$lib/position'

export default interface Level {
	maxGravity: number
	maxAntigravity: number
	defaultGravity: Position[]
	defaultAntigravity: Position[]
	ball: Position & { radius: number }
	hole: Position & { radius: number }
	stars: (Position & { radius: number })[]
	walls: (Position & { width: number; height: number })[]
}
