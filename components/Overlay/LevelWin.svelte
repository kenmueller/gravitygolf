<script lang="ts">
	import hideOverlay from '$lib/overlay/hide'
	import Win from './Win.svelte'

	import RightArrow from '../../images/RightArrow.svelte'

	export let stars: number
	export let hasNext: boolean
	export let next: string
</script>

<Win {stars}>
	<a href={next} on:click={hideOverlay}>
		{#if hasNext}
			Next
			<RightArrow />
		{:else}
			Done
		{/if}
	</a>
</Win>

<style lang="scss">
	a {
		display: flex;
		align-items: center;
		margin-top: 2.5rem;
		padding: 0.5rem 1rem;
		color: white;
		background: colors.$blue;
		border-radius: 0.5rem;
		transition: opacity 0.3s;

		> :global(svg) {
			height: 1.1rem;
			margin-left: 0.8rem;
		}

		&:hover {
			opacity: 0.7;
		}
	}
</style>
