<script lang="ts">
	export let id: number
</script>

<a href="/levels/{id}">Level {id}</a>

<style lang="scss">
	a {
		display: flex;
		justify-content: center;
		align-items: center;
		text-decoration: none;
		color: rgba(white, 0.7);
		background: rgba(white, 0.1);
		border-radius: 0.5rem;
		transition: color 0.3s;

		&:hover {
			color: white;
		}
	}
</style>
