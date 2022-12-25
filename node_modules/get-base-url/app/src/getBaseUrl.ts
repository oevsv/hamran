import { isBrowser } from "browser-or-node"

const dir = "/"
const point = "."
const localHost = "127.0.0.1"


export function getBaseUrl(url: string = "") {
  const urlAsArray = url.split(dir);
  const doubleSlashIndex = url.indexOf("://")
  if (doubleSlashIndex !== -1 && doubleSlashIndex === url.indexOf(dir)-1) {
    return urlAsArray[2]
  }
  else {
    let firstPointIndex = url.indexOf(point)
    if (firstPointIndex === -1) firstPointIndex = NaN
    let firstSlashIndex = url.indexOf(dir)
    if (firstSlashIndex === -1) firstSlashIndex = NaN

    if ((isNaN(firstSlashIndex) && firstPointIndex > 0 && firstPointIndex !== url.length-1) || (firstPointIndex < firstSlashIndex && !(url.startsWith(point) || url.startsWith(dir)))) {
      return urlAsArray[0]
    }
    else return isBrowser ? window.location.host : localHost
  }
}

export default getBaseUrl
