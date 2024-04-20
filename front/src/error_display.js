import { writable } from "svelte/store";

export const ERROR_DISPLAY = writable(undefined);

let error_timeout = undefined;

export function display_error(msg) {
    ERROR_DISPLAY.set({error: msg})
    if (error_timeout !== undefined) {
        clearTimeout(error_timeout);
    }
    error_timeout = setTimeout(() => ERROR_DISPLAY.set(undefined), 5000);
}