<script lang="ts">
	import hideOverlay from '$lib/overlay/hide'
	import Win from './Win.svelte'

	import Reset from '../../images/Reset.svelte'
	import RightArrow from '../../images/RightArrow.svelte'

	export let stars: number
	export let hasNext: boolean
	export let next: string
	export let reset: () => void

	const restart = () => {
		reset()
		hideOverlay()
	}
</script>

<Win {stars}>
	<div>
		<button on:click={restart}>
			<Reset />
			Restart
		</button>
		<a href={next} on:click={hideOverlay}>
			{#if hasNext}
				Next
				<RightArrow />
			{:else}
				Done
			{/if}
		</a>
	</div>
</Win>

<style lang="scss">
	div {
		display: flex;
		margin-top: 2.5rem;
	}

	button,
	a {
		display: flex;
		align-items: center;
		padding: 0.5rem 1rem;
		font-size: 1rem;
		color: white;
		background: colors.$blue;
		border-radius: 0.5rem;
		transition: opacity 0.3s;

		&:hover {
			opacity: 0.7;
		}
	}

	button {
		> :global(svg) {
			height: 1.4rem;
			margin-right: 0.5rem;
			transform: translateY(-1px);
		}
	}

	a {
		margin-left: 1rem;

		> :global(svg) {
			height: 1.1rem;
			margin-left: 0.8rem;
		}
	}
</style>
