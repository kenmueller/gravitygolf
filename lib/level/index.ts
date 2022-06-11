export default interface Level {
	gravity: number
	antigravity: number
	ball: [number, number, number]
	hole: [number, number, number]
	walls: [number, number, number, number][]
}
