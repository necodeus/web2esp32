const ENTER_KC = 13
const UP_KC = 38
const DOWN_KC = 40

class TerminalManager {
    constructor({
        wsManager,
        inputSelector,
        listSelector,
    }) {
        this.ws = wsManager
        this.ws.ws.onmessage = (event) => {
            const data = JSON.parse(event.data)
            this.print(data.message, data.isError)
            this.scrollListToBottom()
        }
        this.inputElement = document.querySelector(inputSelector)
        this.inputElement.addEventListener('keydown', this.handleKeyDown.bind(this))
        this.listElement = document.querySelector(listSelector)
        this.commandsIndex = 0
        this.commands = []
    }

    saveCommand(command) {
        this.commands.push(command)
        this.setCommandIndex(this.commands.length)
    }

    clearInput() {
        this.inputElement.value = ''
    }

    setCommandIndex(index) {
        this.commandsIndex = index
    }

    setInputValue(value) {
        this.inputElement.value = value
    }

    restorePreviousCommand() {
        this.setCommandIndex(Math.max(0, this.commandsIndex - 1))
        this.setInputValue(this.commands[this.commandsIndex] || '')
    }

    restoreNextCommand() {
        this.setCommandIndex(Math.min(this.commands.length, this.commandsIndex + 1))
        this.setInputValue(this.commands[this.commandsIndex] || '')
    }

    scrollListToBottom() {
        this.listElement.scrollTop = this.listElement.scrollHeight
    }
    
    handleKeyDown(event) {
        switch (event.keyCode) {
            case ENTER_KC: {
                if (!event.target.value) {
                    return
                }
        
                this.print(event.target.value, null)
                this.scrollListToBottom()
                this.saveCommand(event.target.value)
                this.ws.send(event.target.value)
                this.clearInput()

                break
            }
            case UP_KC: {
                if (!this.commands.length) {
                    return
                }

                this.restorePreviousCommand()

                break
            }
            case DOWN_KC: {
                if (!this.commands.length) {
                    return
                }

                this.restoreNextCommand()

                break
            }
        }
    }

    print(message, isError = null) {
        const div = document.createElement('div')

        div.classList.add('command')
        div.textContent = message

        if (isError === true) {
            div.classList.add('error')
        }

        if (isError === false) {
            div.classList.add('success')
        }

        this.listElement.appendChild(div)
    }
}
