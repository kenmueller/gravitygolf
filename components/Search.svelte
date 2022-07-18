<script lang="ts">
	import Search from '../images/Search.svelte'

	export let placeholder: string
	export let value: string
	export let focus = false

	let input: HTMLInputElement | null = null
	$: if (focus) input?.focus()
</script>

<div>
	<input {placeholder} bind:this={input} bind:value />
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
		background: rgba(white, 0.1);
		border-radius: 0.5rem;
		transition: background 0.3s;

		&::placeholder {
			color: rgba(white, 0.5);
		}

		&:hover,
		&:focus {
			background: rgba(white, 0.2);

			+ :global(svg) {
				opacity: 1;
			}
		}
	}

	div > :global(svg) {
		pointer-events: none;
		position: absolute;
		top: 0.6rem;
		left: 0.6rem;
		width: 1.3rem;
		color: white;
		opacity: 0.7;
		transition: opacity 0.3s;
	}
</style>
