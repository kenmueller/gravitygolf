import type { SvelteComponentTyped } from 'svelte'

export default interface Overlay {
	component: SvelteComponentTyped
	props: Record<string, unknown>
}
