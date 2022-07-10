<script lang="ts">
	import { blur } from 'svelte/transition'

	export let url: URL
	export let hidden = false
	export let duration = 500
</script>

{#key url}
	<div
		aria-hidden={hidden || undefined}
		in:blur={{ duration, delay: duration }}
		out:blur={{ duration }}
	>
		<slot />
	</div>
{/key}

<style lang="scss">
	div {
		height: 100%;
	}
</style>
