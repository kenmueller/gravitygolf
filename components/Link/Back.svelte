<script lang="ts">
	import Back from '../../images/Back.svelte'

	export let href: string
</script>

<a {href} aria-label="Back">
	<Back />
	<slot />
</a>

<style lang="scss">
	a {
		display: flex;
		align-items: center;
		color: rgba(white, 0.7);
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			margin: -0.5rem 0 -0.5rem -0.7rem;
			width: 2.5rem;
		}
	}
</style>
