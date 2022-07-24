import type Position from '$lib/position'

export default interface Level {
	maxGravity: number
	maxAntigravity: number
	fixedGravity: Position[]
	fixedAntigravity: Position[]
	ball: Position & { radius: number; coeff_restitution: number }
	hole: Position & { radius: number }
	stars: (Position & { radius: number })[]
	walls: (Position & { width: number; height: number })[]
	message: string | null
}
