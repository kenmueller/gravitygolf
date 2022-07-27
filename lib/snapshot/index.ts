export default interface Snapshot {
	id: string
	exists: boolean | (() => boolean)
	get(key: string): unknown
	data(): Record<string, unknown> | undefined
}
