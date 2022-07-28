import type RawLevel from '$lib/level/raw'

export default interface EditorSceneOptions {
	defaultName: string | null
	publishLink: string | null
	initialData: RawLevel | null
}
