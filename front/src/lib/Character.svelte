<script>
export let character;
const input_id = `${character.name}`
export let onchange;
</script>

<div>
    <label for={input_id}>
        <img src={`${character.class}.png`} alt={character.class}>
        <span>{character.name}</span>
    </label>
    <input type="checkbox" id={input_id} name={input_id} on:change={onchange}>
</div>

<style>
div {
    place-self: center;
    user-select: none;
    display: flex;
}

label {
    display: inline-flex;
    margin-right: 0.3em;
}

img {
    height: 24px;
    margin-right: 0.3em;
}

</style>