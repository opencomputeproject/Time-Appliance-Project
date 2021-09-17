/*
Copyright (c) Facebook, Inc. and its affiliates.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package clientgenlib

import (
	"time"
	"container/heap"
)

// Time Item, used for Heap operations
type timeItem struct {
	value interface{}

	// for heap
	priority time.Time
	index    int // index of item in heap
}

// A heap based on items ordered by time.Time
type timeHeap struct {
	Items []*timeItem
}

// Len returns number of items in the heap
func (h *timeHeap) Len() int { return len(h.Items) }

// Less compares two items in the heap at provides indices
func (h *timeHeap) Less(i, j int) bool {
	if i > len(h.Items) || j > len(h.Items) ||
		i < 0 || j < 0 {
		return false
	}
	return h.Items[i].priority.Before(h.Items[j].priority)
}

// Swap swaps two items in the heap at the given indices
func (h *timeHeap) Swap(i, j int) {
	h.Items[i], h.Items[j] = h.Items[j], h.Items[i]
	h.Items[i].index = i
	h.Items[j].index = j
}

// Push adds an item to the heap, required for heap interface. Call SafeUpdate instead
func (h *timeHeap) Push(x interface{}) {
	if len(h.Items) == cap(h.Items) {
		// grow the capacity
		newSlice := make([]*timeItem, len(h.Items), cap(h.Items)+100)
		copy(newSlice, h.Items)
		h.Items = newSlice
	}
	h.Items = h.Items[:len(h.Items)+1] // extend slice by one
	n := h.Len() - 1
	item := x.(*timeItem)
	item.index = n
	h.Items[n] = item
}

// Pop removes the min item from the heap, required for heap interface. Call SafePop instead
func (h *timeHeap) Pop() interface{} {
	if h.Len() == 0 {
		return nil
	}

	n := h.Len()
	item := h.Items[n-1]
	h.Items[n-1] = nil
	item.index = -1

	h.Items = h.Items[:n-1] //reduce slide size
	return item
}

// Peek returns a pointer from min item without removing it, required for heap interface. Call SafePeek instead
func (h *timeHeap) Peek() interface{} {
	if h.Len() == 0 {
		return nil
	}
	return h.Items[0]
}

// Update edits an item in the heap already, required for heap interface. Call SafeUpdate instead
func (h *timeHeap) Update(item *timeItem, value interface{}, priority time.Time) bool {
	if item.index == -1 {
		return false
	}
	item.value = value
	item.priority = priority
	heap.Fix(h, item.index)
	return true
}

// Init initializes a given timeHeap pointer with a given starting capacity
func (h *timeHeap) Init(startCapacity int) {
	h.Items = make([]*timeItem, 0, startCapacity)
	heap.Init(h)
}

// SafePush pushes a *timeItem into the provides timeHeap
func (h *timeHeap) SafePush(x interface{}) {
	if x.(*timeItem).index == -1 {
		heap.Push(h, x)
	} else {
		item := x.(*timeItem)
		h.Update(item, item.value, item.priority)
	}
}

// SafePop returns the earliest *timeItem in the timeHeap and removes it from the heap
func (h *timeHeap) SafePop() interface{} {
	val := heap.Pop(h)
	return val
}

// SafePeek returns the earliest *timeItem in the timeHeap
func (h *timeHeap) SafePeek() interface{} {
	val := h.Peek()
	return val
}

// SafeUpdate edits an item in the heap with a given value and time, or inserts it if not present in the heap
// returns true if it was in the heap, false if it was inserted into the heap
func (h *timeHeap) SafeUpdate(item *timeItem, value interface{}, priority time.Time) bool {
	val := h.Update(item, value, priority)
	if !val { // wasn't actually in the heap yet
		item.value = value
		item.priority = priority
		heap.Push(h, item)
	}
	return val
}

// SafeRemove removes a given item from the timeHeap
func (h *timeHeap) SafeRemove(item *timeItem) {
	if item == nil {
		return
	}
	if item.index != -1 {
		heap.Remove(h, item.index)
	}
}
