class SdCardManager {
    constructor({
        isSecure = false,
        host,
        port,
        currentPathSelector = '/',
    }) {
        this.isSecure = isSecure
        this.host = host
        this.port = port
        this.currentPath = '/'
        this.currentPathElement = document.querySelector(currentPathSelector)

        this.getFiles(this.currentPath)
            .then (files => {
                this.updateList(files)
                this.updatePath(this.currentPath)
                this.currentPathElement.innerHTML = this.currentPath
            })

        document.querySelector('#add-dir').addEventListener('click', () => {
            this.createDirectory()
                .then(data => {
                    console.log('Po dodaniu', data)

                    this.getFiles(this.currentPath)
                        .then(files => {
                            this.updateList(files)
                            this.updatePath(this.currentPath)
                            this.currentPathElement.innerHTML = this.currentPath
                        })
                })
        })

        // on #file change, submit the form to current path
        document.querySelector('#file').addEventListener('change', async (event) => {
            const path = this.currentPath.endsWith('/') ? this.currentPath : `${this.currentPath}/`;
            const file = event.target.files[0]

            const formData = new FormData()
            formData.append('path', path)
            formData.append('file', file)

            fetch(`${this.isSecure ? 'https' : 'http'}://${this.host}:${this.port}/upload`, {
                method: 'POST',
                body: formData
            })
                .then(data => {
                    console.log('Po dodaniu', data)

                    this.getFiles(this.currentPath)
                        .then(files => {
                            this.updateList(files)
                            this.updatePath(this.currentPath)
                            this.currentPathElement.innerHTML = this.currentPath
                        })
                })
        })
    }

    async getFiles(path = '/') {
        return fetch(`${this.isSecure ? 'https' : 'http'}://${this.host}:${this.port}/list?path=${path}`)
            .then(response => response.json())
    }

    updatePath(path) {
        this.currentPath = path
    }

    updateList(files) {
        const list = document.querySelector('#sdcard-list')
        list.innerHTML = ''

        if (this.currentPath !== '/') {
            const folder = document.createElement('div')
            folder.classList.add('file')
            folder.innerHTML = `
                <div style="display: flex; align-items: center;">
                    <img width="18px" height="18px" src="/icons/folder.svg" />
                    <span>..</span>
                </div>
            `
            folder.addEventListener('click', () => {
                this.back()
            })
            list.appendChild(folder)
        }

        files.forEach(file => {
            console.log(file)

            const element = document.createElement('div')
            element.classList.add('file')

            if (file.type === 'directory') {
                element.innerHTML = `
                    <div style="width: 100%; display: flex; justify-content: space-between; align-items: center;">
                        <div style="display: flex; align-items: center;">
                            <img width="18px" height="18px" src="/icons/folder.svg" />
                            <span>${file.name}</span>
                        </div>
                        <div style="display: flex; align-items: center;" class="delete">
                            <img src="/icons/trash.svg" />
                        </div>
                    </div>
                `

                element.querySelector('.delete').addEventListener('click', async (event) => {
                    event.stopPropagation()

                    const path = this.currentPath.endsWith('/') ? this.currentPath : `${this.currentPath}/`;
                    const filePath = `${path}${file.name}`

                    fetch(`${this.isSecure ? 'https' : 'http'}://${this.host}:${this.port}/delete?path=${filePath}`)
                        .then(data => {
                            console.log('Po usunięciu', data)

                            this.getFiles(this.currentPath).then(files => {
                                this.updateList(files)
                                this.updatePath(this.currentPath)
                                this.currentPathElement.innerHTML = this.currentPath
                            })
                        })

                    this.updatePreviewToNotSelected()
                })

                element.addEventListener('click', () => {
                    if (this.currentPath.endsWith('/')) {
                        this.currentPath = `${this.currentPath}${file.name}`
                    } else {
                        this.currentPath = `${this.currentPath}/${file.name}`
                    }
            
                    this.getFiles(this.currentPath).then(files => {
                        this.updateList(files)
                        this.updatePath(this.currentPath)
                        this.currentPathElement.innerHTML = this.currentPath

                        this.updatePreviewToNotSelected()
                    })
                })
            } else {
                element.innerHTML = `
                    <div style="width: 100%; display: flex; justify-content: space-between; align-items: center;">
                        <div style="display: flex; align-items: center;">
                            <img src="/icons/file.svg" />
                            <span>${file.name}</span>
                        </div>
                        <div style="display: flex; align-items: center;" class="delete">
                            <img src="/icons/trash.svg" />
                        </div>
                    </div>
                `

                element.addEventListener('click', async () => {
                    const path = this.currentPath.endsWith('/') ? this.currentPath : `${this.currentPath}/`;
                    const filePath = `${path}${file.name}`

                    this.getPreview(filePath).then(fileDetails => {
                        this.updatePreview(fileDetails)
                    })
                })

                element.querySelector('.delete').addEventListener('click', async (event) => {
                    event.stopPropagation()

                    const path = this.currentPath.endsWith('/') ? this.currentPath : `${this.currentPath}/`;
                    const filePath = `${path}${file.name}`

                    fetch(`${this.isSecure ? 'https' : 'http'}://${this.host}:${this.port}/delete?path=${filePath}`)
                        .then(data => {
                            console.log('Po usunięciu', data)

                            this.getFiles(this.currentPath).then(files => {
                                this.updateList(files)
                                this.updatePath(this.currentPath)
                                this.currentPathElement.innerHTML = this.currentPath

                                this.updatePreviewToNotSelected()
                            })
                        })
                })
            }

            list.appendChild(element)
        })

        if (files.length === 0) {
            const element = document.createElement('div')
            element.innerHTML = `
                <div class="file" style="height: 30px">
                    This folder is empty. Add some files!
                </div>
            `
            list.appendChild(element)
        }
    }

    async back() {
        this.currentPath = this.currentPath.split('/').slice(0, -1).join('/') || '/'
        
        this.getFiles(this.currentPath).then(files => {
            this.updateList(files)
            this.updatePath(this.currentPath)
            this.currentPathElement.innerHTML = this.currentPath
        })
    }

    async createDirectory() {
        const name = prompt('Enter directory name').trim()

        if (!name) {
            alert('Directory name cannot be empty!')
            return Promise.resolve()
        }

        const path = this.currentPath.endsWith('/') ? this.currentPath : `${this.currentPath}/`;

        return fetch(`${this.isSecure ? 'https' : 'http'}://${this.host}:${this.port}/mkdir?path=${path}${name}`)
    }

    async getPreview(path) { // should be called fileDetails, but later on
        return fetch(`${this.isSecure ? 'https' : 'http'}://${this.host}:${this.port}/preview?path=${path}`)
            .then(response => response.json())
    }

    updatePreview(fileDetails) {
        const previewContainer = document.querySelector('#preview-container')

        previewContainer.innerHTML = ''

        const table = document.createElement('table')
        table.style.width = '50%'

        const name = document.createElement('tr')

        name.innerHTML = `
            <th style="font-weight: bold; text-align: left;">Name:</th>
            <td style="padding: 5px 10px;">${fileDetails.name}</td>
        `
        table.appendChild(name)

        const size = document.createElement('tr')
        size.innerHTML = `
            <th style="font-weight: bold; text-align: left;">Size:</th>
            <td style="padding: 5px 10px;">${fileDetails.size}</td>
        `
        table.appendChild(size)

        const type = document.createElement('tr')
        type.innerHTML = `
            <th style="font-weight: bold; text-align: left;">Type:</th>
            <td style="padding: 5px 10px;">${fileDetails.contentType}</td>
        `
        table.appendChild(type)

        const createdAt = document.createElement('tr')
        createdAt.innerHTML = `
            <th style="font-weight: bold; text-align: left;">Modified at:</th>
            <td style="padding: 5px 10px;">${fileDetails.modifiedAt}</td>
        `
        table.appendChild(createdAt)

        previewContainer.appendChild(table)

        const download = document.createElement('div')
        download.style.width = '50%'
        download.innerHTML = `
            <img src="/icons/download.svg" style="display: block; width: 18px; height: 18px; margin: 0 auto;" />
        `
    }

    updatePreviewToNotSelected() {
        const previewContainer = document.querySelector('#preview-container')

        previewContainer.innerHTML = 'No file selected'
    }
}