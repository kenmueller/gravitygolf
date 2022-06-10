<script lang="ts">
	import Back from '../../images/Back.svelte'

	export let href: string
</script>

<a {href}>
	<Back />
	<slot />
</a>

<style lang="scss">
	a {
		display: flex;
		align-items: center;
		text-decoration: none;
		color: rgba(white, 0.7);
		transition: color 0.3s;

		&:hover {
			color: white;
		}

		> :global(svg) {
			display: block;
			width: 2.5rem;
		}
	}
</style>
