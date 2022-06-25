export default interface RawLevel {
	gravity?: number | [number, [number, number][]]
	antigravity?: number | [number, [number, number][]]
	ball: [number, number, number]
	hole: [number, number, number]
	stars?: [number, number, number][]
	walls?: [number, number, number, number][]
}
