<script lang="ts">
	import { blur } from 'svelte/transition'

	import overlay from '$lib/overlay/store'

	export let duration = 500
</script>

{#if $overlay}
	<div role="dialog" transition:blur={{ duration }}>
		<svelte:component this={$overlay.component} {...$overlay.props} />
	</div>
{/if}

<style lang="scss">
	div {
		position: absolute;
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
		top: 0;
		right: 0;
		bottom: 0;
		left: 0;
		backdrop-filter: blur(4px);
		z-index: 200;
	}
</style>
