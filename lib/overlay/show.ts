import type { SvelteComponentTyped } from 'svelte'

import overlay from './store'

const showOverlay = (
	component: unknown,
	props: Record<string, unknown> = {}
) => {
	overlay.set({
		component: component as SvelteComponentTyped,
		props
	})
}

export default showOverlay
