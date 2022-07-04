<script lang="ts">
	import Search from '../images/Search.svelte'

	export let placeholder: string
	export let value: string
</script>

<div>
	<input {placeholder} bind:value />
	<Search />
</div>

<style lang="scss">
	div {
		position: relative;
	}

	input {
		display: block;
		width: 100%;
		padding: 0.5rem 1rem 0.5rem 2.5rem;
		font-size: 1rem;
		color: white;
		background: transparentize(white, 0.9);
		border-radius: 0.5rem;
		transition: background 0.3s;

		&::placeholder {
			color: transparentize(white, 0.5);
		}

		&:hover,
		&:focus {
			background: transparentize(white, 0.8);

			+ :global(svg) {
				opacity: 1;
			}
		}
	}

	div > :global(svg) {
		position: absolute;
		top: 0.6rem;
		left: 0.6rem;
		width: 1.3rem;
		color: white;
		opacity: 0.7;
		transition: opacity 0.3s;
	}
</style>
