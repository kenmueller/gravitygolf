<script lang="ts">
	import { blur } from 'svelte/transition'

	import overlay from '$lib/overlay/store'

	export let duration = 500
</script>

{#if $overlay}
	<div class="outer" role="dialog" transition:blur={{ duration }}>
		<div class="inner">
			<svelte:component this={$overlay.component} {...$overlay.props} />
		</div>
	</div>
{/if}

<style lang="scss">
	div {
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
	}

	.outer {
		position: absolute;
		top: 0;
		right: 0;
		bottom: 0;
		left: 0;
		backdrop-filter: blur(4px);
		z-index: 200;
	}

	.inner {
		width: 95%;
		height: 95%;
	}
</style>
