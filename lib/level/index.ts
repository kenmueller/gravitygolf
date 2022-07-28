import type Position from '$lib/position'

export default interface Level {
	/** Does not include fixed gravities. */
	maxGravity: number

	/** Does not include fixed antigravities. */
	maxAntigravity: number

	fixedGravity: Position[]
	fixedAntigravity: Position[]
	ball: Position & { radius: number }
	hole: Position & { radius: number }
	stars: (Position & { radius: number })[]
	walls: (Position & { width: number; height: number })[]
}
