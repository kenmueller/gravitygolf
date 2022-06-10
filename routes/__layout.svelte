<script context="module" lang="ts">
	export const load: Load = ({ url }) => ({
		props: { url }
	})
</script>

<script lang="ts">
	import type { Load } from '@sveltejs/kit'
	import { blur } from 'svelte/transition'

	const duration = 500

	export let url: URL
</script>

{#key url}
	<div in:blur={{ duration, delay: duration }} out:blur={{ duration }}>
		<slot />
	</div>
{/key}

<style lang="scss" global>
	*,
	::before,
	::after {
		box-sizing: border-box;
		margin: 0;
		padding: 0;
		font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen,
			Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;
		border: none;
		outline: none;
	}

	html,
	body,
	div {
		height: 100%;
	}
</style>
